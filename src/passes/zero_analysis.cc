#include "../control_flow.hh"
#include "../data_flow_analysis.hh"
#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    State z_flow(Node inst, State incoming_state) {
        if (inst == Assign) {
            std::string ident = get_identifier(inst / Ident);
            Node rhs = inst / Rhs / Expr;

            if (rhs == Int) {
                incoming_state[ident].type =
                    get_int_value(rhs) == 0 ? TZero : TNonZero;
            } else if (rhs == Ident) {
                std::string rhs_ident = get_identifier(rhs);
                incoming_state[ident] = incoming_state[rhs_ident];
            } else {
                // Either rhs is operation or input, regerdless top is reached
                incoming_state[ident].type = TTop;
            }
        }
        return incoming_state;
    }

    StateValue z_join(StateValue st1, StateValue st2) {
        StateValue top = {TTop, 0};
        StateValue zero = {TZero, 0};
        StateValue nzero = {TNonZero, 0};

        if (st1.type == TBottom) {
            return st2;
        }
        if (st2.type == TBottom) {
            return st1;
        }

        if (st1.type == TTop || st2.type == TTop) {
            return top;
        }

        if (st1.type == TZero && st2.type == TZero) {
            return zero;
        }
        if (st1.type == TNonZero && st2.type == TNonZero) {
            return nzero;
        }

        return top;
    }

    // clang-format off
    PassDef z_analysis(std::shared_ptr<ControlFlow> control_flow) {

        PassDef z_analysis =  {
            "z_analysis",
            normalization_wf,
            dir::topdown | dir::once,
            {
                // Control flow inside of While
                T(While)[While] >> [control_flow](Match &_) -> Node
                    {
                        auto b_expr = _(While) / BExpr;
                        auto body = _(While) / Do;

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

						control_flow->add_successor(get_last_basic_children(prev), get_first_basic_child(post));

                        return NoChange;
                    },
			}
		};

        // clang-format on
        z_analysis.post([control_flow](Node) {
            const Nodes instructions = control_flow->get_instructions();
            const Vars vars = control_flow->get_vars();

            auto analysis =
                std::make_shared<DataFlowAnalaysis>(instructions, vars);

            // Initial state has all unknown, i.e. all Top
            analysis->set_state(instructions[0], TTop, 0);

            std::deque<Node> worklist{instructions[0]};

            while (!worklist.empty()) {
                Node inst = worklist.front();
                worklist.pop_front();

                State in_state = analysis->get_state_in_table(inst);
                State out_state = z_flow(inst, in_state);

                analysis->set_state_in_table(inst, out_state);

                for (Node succ : control_flow->successors(inst)) {
                    State succ_state = analysis->get_state_in_table(succ);
                    State new_succ_state =
                        analysis->join(out_state, succ_state, z_join);

                    if (!state_equals(new_succ_state, succ_state)) {
                        analysis->set_state_in_table(succ, new_succ_state);
                        worklist.push_back(succ);
                    }
                }
            }

            control_flow->log_instructions();
            analysis->log_state_table(instructions);

            return 0;
        });

        return z_analysis;
    }
}
