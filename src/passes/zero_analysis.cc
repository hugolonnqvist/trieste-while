#include <memory>

#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    auto instructions = std::make_shared<Nodes>();
    auto vars = std::make_shared<std::set<std::string>>();

    // For Zero analysis, we have the following lattice precedence,
    // where join_table(t1, t2) = t1 join t2:
    const JoinTable join_table = std::map<Token, std::map<Token, Token>>{
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

    State zero_flow_function(Node inst, State incoming_state) {
        if (inst == Assign) {
            std::string ident = get_identifier(inst / Ident);
            Node rhs = inst / AExpr / Expr;

            if (rhs == Int) {
                int value = get_int_value(rhs);
                if (value == 0) {
                    incoming_state[ident] = TZero;
                } else {
                    incoming_state[ident] = TNonZero;
                }
            } else if (rhs == Ident) {
                std::string rhs_ident = get_identifier(rhs / Ident);
                incoming_state[ident] = incoming_state[rhs_ident];
            } else {
                // Either rhs is operation or input, regerdless top is reached
                incoming_state[ident] = TTop;
            }
        }
        return incoming_state;
    }

    PassDef init_flow_graph() {
        auto predecessor = std::make_shared<NodeMap<NodeSet>>();
        auto successor = std::make_shared<NodeMap<NodeSet>>();

        // clang-format off
        PassDef init_flow_graph =  {
            "init_flow_graph",
            statements_wf,
            dir::topdown | dir::once,
            {
                // Control flow inside of While
                T(While)[While] >>
                    [predecessor, successor](Match &_) -> Node
                    {
                        auto b_expr = _(While) / BExpr;
                        auto body = _(While) / Do;

                        add_predecessor(predecessor, b_expr, get_last_basic_children(body));
                        add_predecessor(predecessor, get_first_basic_child(body), b_expr);

						add_predecessor(successor, b_expr, get_first_basic_child(body));
						add_predecessor(successor, get_last_basic_children(body), b_expr);

                        return NoChange;
                    },

                // Control flow inside of If
                T(If)[If] >> 
                    [predecessor, successor](Match &_) -> Node
                    {
                        auto b_expr = _(If) / BExpr;
                        auto then_stmt = _(If) / Then;
                        auto else_stmt = _(If) / Else;

                        add_predecessor(predecessor, get_first_basic_child(then_stmt), b_expr);
                        add_predecessor(predecessor, get_first_basic_child(else_stmt), b_expr);

						add_predecessor(successor, b_expr, get_first_basic_child(then_stmt));
						add_predecessor(successor, b_expr, get_first_basic_child(else_stmt));

                        return NoChange;
                    },

                // General case of a sequence of statements
                In(Semi) * T(Stmt)[Prev] * T(Stmt)[Stmt] >>
                    [predecessor, successor](Match &_) -> Node
                    {

                        auto node = get_first_basic_child(_(Stmt));
                        auto prev_nodes = get_last_basic_children(_(Prev));

                        add_predecessor(predecessor, node, prev_nodes);

						auto nodes = get_last_basic_children(_(Prev));
                        add_predecessor(successor, nodes, get_first_basic_child(_(Stmt)));


                        return NoChange;
                    },

        }};

        // clang-format on
        init_flow_graph.post([predecessor, successor](Node) {
            NodeMap<State> state_table = NodeMap<State>();

            set_all_states_to_bottom(instructions, vars, state_table);

            // Init state, with all TTOP
            State init_state = State();
            for (auto it = vars->begin(); it != vars->end(); it++) {
                init_state.insert({*it, TTop});
            }

            state_table.insert({instructions->at(0), init_state});

            std::deque<Node> worklist;
            worklist.push_back(instructions->at(0));

            while (!worklist.empty()) {
                Node inst = worklist.front();
                worklist.pop_front();

                State incoming_state = state_table[inst];
                State output = zero_flow_function(inst, incoming_state);
                state_table[inst] = output;

                for (Node succ : (*successor)[inst]) {
                    State succ_state = state_table[succ];
                    State union_state = join(output, succ_state, join_table);

                    if (!states_equal(union_state, succ_state)) {
                        state_table[succ] = union_state;
                        worklist.push_back(succ);
                    }
                }
            }

            // Printing
            log_instructions(*instructions);
            log_state_table(*instructions, state_table);
            // log_predecessors_and_successors(instructions, predecessor,
            //                                 successor);
            return 0;
        });

        return init_flow_graph;
    }
    // clang-format off
    PassDef gather_instructions() {
		return {
            "gather_instructions",
            statements_wf,
            dir::topdown | dir::once,
            {
                T(Assign, Skip)[Inst] >>
                    [](Match &_) -> Node
                    {
                        Node inst = _(Inst);
                        instructions->push_back(_(Inst));

                        // Gather variables
                        if (inst->type() == Assign) {
                            auto ident = inst / Ident;
                            vars->insert(get_identifier(ident));
                        }

                        return NoChange;
                    },
                In(While, If) * T(BExpr)[Inst] >>
                    [](Match &_) -> Node
                    {
                        instructions->push_back(_(Inst));
                        return NoChange;
                    },
			}};
    }
}
