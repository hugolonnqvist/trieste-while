#include "../control_flow.hh"
#include "../data-flow-analysis.hh"
#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    // For Zero analysis, we have the following lattice precedence,
    // where join_table(t1, t2) = t1 join t2:
    JoinTable join_table = std::map<Token, std::map<Token, Token>>{
        {TTop,
         {
             {TTop, TTop},
             {TZero, TTop},
             {TNonZero, TTop},
             {TBottom, TTop},
         }},
        {TZero,
         {
             {TTop, TTop},
             {TZero, TZero},
             {TNonZero, TTop},
             {TBottom, TZero},
         }},
        {TNonZero,
         {
             {TTop, TTop},
             {TZero, TTop},
             {TNonZero, TNonZero},
             {TBottom, TNonZero},
         }},
        {TBottom,
         {
             {TTop, TTop},
             {TZero, TZero},
             {TNonZero, TNonZero},
             {TBottom, TBottom},
         }},
    };
    auto z_analysis = std::make_shared<DataFlowAnalaysis>(join_table);

    State flow_function(Node inst, State incoming_state) {
        if (inst == Assign) {
            std::string ident = get_identifier(inst / Ident);
            Node rhs = inst / Rhs / Expr;

            if (rhs == Int) {
                incoming_state[ident] =
                    get_int_value(rhs) == 0 ? TZero : TNonZero;
            } else if (rhs == Ident) {
                std::string rhs_ident = get_identifier(rhs);
                incoming_state[ident] = incoming_state[rhs_ident];
            } else {
                // Either rhs is operation or input, regerdless top is reached
                incoming_state[ident] = TTop;
            }
        }
        return incoming_state;
    }

    PassDef init_flow_graph(std::shared_ptr<ControlFlow> control_flow) {
        // clang-format off
        PassDef init_flow_graph =  {
            "init_flow_graph",
            normalization_wf,
            dir::topdown | dir::once,
            {
                // Control flow inside of While
                T(While)[While] >>
                    [control_flow](Match &_) -> Node
                    {
                        auto b_expr = _(While) / BExpr;
                        auto body = _(While) / Do;

                        control_flow->add_predecessor(b_expr, get_last_basic_children(body));
                        control_flow->add_predecessor(get_first_basic_child(body), b_expr);

						control_flow->add_successor(b_expr, get_first_basic_child(body));
						control_flow->add_successor(get_last_basic_children(body), b_expr);

                        return NoChange;
                    },

                // Control flow inside of If
                T(If)[If] >> 
                    [control_flow](Match &_) -> Node
                    {
                        auto b_expr = _(If) / BExpr;
                        auto then_stmt = _(If) / Then;
                        auto else_stmt = _(If) / Else;

                        control_flow->add_predecessor(get_first_basic_child(then_stmt), b_expr);
                        control_flow->add_predecessor(get_first_basic_child(else_stmt), b_expr);

						control_flow->add_successor(b_expr, get_first_basic_child(then_stmt));
						control_flow->add_successor(b_expr, get_first_basic_child(else_stmt));
				
                        return NoChange;
                    },

                // General case of a sequence of statements
                In(Semi) * T(Stmt)[Prev] * T(Stmt)[Post] >>
                    [control_flow](Match &_) -> Node
                    {

						auto prev = _(Prev);
						auto post = _(Post);

						control_flow->add_predecessor(get_first_basic_child(post), get_last_basic_children(prev));
						control_flow->add_successor(get_last_basic_children(prev), get_first_basic_child(post));

                        return NoChange;
                    },

        }};

        // clang-format on
        init_flow_graph.post([control_flow](Node) {
            const Nodes instructions = control_flow->get_instructions();
            z_analysis->set_all_states_to_bottom(instructions);

            // Initial state has all unknown, i.e. all Top
            z_analysis->set_state(instructions[0], TTop);

            std::deque<Node> worklist{instructions[0]};

            while (!worklist.empty()) {
                Node inst = worklist.front();
                worklist.pop_front();

                State incoming_state = z_analysis->get_state_in_table(inst);
                State outgoing_state = flow_function(inst, incoming_state);
                z_analysis->set_state_in_table(inst, outgoing_state);

                for (Node succ : control_flow->successors(inst)) {
                    State succ_state = z_analysis->get_state_in_table(succ);
                    State new_succ_state =
                        z_analysis->join(outgoing_state, succ_state);

                    if (new_succ_state != succ_state) {
                        z_analysis->set_state_in_table(succ, new_succ_state);
                        worklist.push_back(succ);
                    }
                }
            }

            control_flow->log_instructions();
            z_analysis->log_state_table(instructions);

            return 0;
        });

        return init_flow_graph;
    }
    // clang-format off
    PassDef gather_instructions(std::shared_ptr<ControlFlow> control_flow) {
		PassDef gather_instructions = {
            "gather_instructions",
            normalization_wf,
            dir::topdown | dir::once,
            {
                T(Assign, Skip, Output)[Inst] >>
                    [control_flow](Match &_) -> Node
                    {
                        Node inst = _(Inst);
						control_flow->add_instruction(inst);

                        // Gather variables
                        if (inst->type() == Assign) {
                            auto ident = inst / Ident;
							z_analysis->add_var(ident);
                        }

                        return NoChange;
                    },

                In(While, If) * T(BExpr)[Inst] >>
                    [control_flow](Match &_) -> Node
                    {
                        control_flow->add_instruction(_(Inst));
                        return NoChange;
                    },
			}
		};

		gather_instructions.post([control_flow](Node) {
			if (control_flow->get_instructions().empty()) {
				throw std::runtime_error("Unexpected, missing instructions");
			}
			
			return 0;	
		});
		return gather_instructions;
    }
}
