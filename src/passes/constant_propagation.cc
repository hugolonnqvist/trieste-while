#include "../data_flow_analysis.hh"
#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    StateValue atom_flow_helper(Node inst, State incoming_state) {
        if (inst == Atom) {
            Node expr = inst / Expr;

            if (expr == Int) {
                return {TConstant, get_int_value(expr)};
            } else if (expr == Ident) {
                std::string rhs_ident = get_identifier(expr);
                return incoming_state[rhs_ident];
            } else {
                // Rhs is input, top is reached
                return {TTop, 0};
            }
        }

        return {TTop, 0};
    }

    State cp_flow_fn(Node inst, State incoming_state) {
        if (inst == Assign) {
            std::string ident = get_identifier(inst / Ident);

            if ((inst / Rhs) == Atom) {
                incoming_state[ident] =
                    atom_flow_helper(inst / Rhs, incoming_state);
            } else {
                Node op = (inst / Rhs) / Expr;
                Node lhs = op / Lhs;
                Node rhs = op / Rhs;

                StateValue lhs_st = atom_flow_helper(lhs, incoming_state);
                StateValue rhs_st = atom_flow_helper(rhs, incoming_state);

                if (lhs_st.type == TConstant && rhs_st.type == TConstant) {
                    int calculated_value =
                        op == Add   ? lhs_st.value + rhs_st.value
                        : op == Sub ? lhs_st.value - rhs_st.value
                                    : lhs_st.value * rhs_st.value;
                    incoming_state[ident] = {TConstant, calculated_value};
                } else {
                    incoming_state[ident] = {TTop, 0};
                }
            }
        }
        return incoming_state;
    }

    StateValue cp_join_fn(StateValue st1, StateValue st2) {
        StateValue top_elem = {TTop, 0};
        if (st1.type == TBottom) {
            return st2;
        } else if (st2.type == TBottom) {
            return st1;
        }

        if (st1.type == TTop || st2.type == TTop) {
            return top_elem;
        }

        if (st1.type == TConstant && st2.type == TConstant) {
            if (st1.value == st2.value) {
                return st1;
            } else {
                return top_elem;
            }
        }

        return top_elem;
    }

    // clang-format off
    PassDef constant_propagation(std::shared_ptr<ControlFlow> control_flow) {
		PassDef constant_propagation = {
			"constant_propagation",
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
        constant_propagation.post([control_flow](Node) {
            const Nodes instructions = control_flow->get_instructions();
            const Vars vars = control_flow->get_vars();

            auto cp_analysis =
                std::make_shared<DataFlowAnalaysis>(instructions, vars);

            // Initial state has all unknown, i.e. all Top
            cp_analysis->set_state(instructions[0], TTop, 0);

            std::deque<Node> worklist{instructions[0]};

            while (!worklist.empty()) {
                Node inst = worklist.front();
                worklist.pop_front();

                State incoming_state = cp_analysis->get_state_in_table(inst);
                State outgoing_state = cp_flow_fn(inst, incoming_state);

                cp_analysis->set_state_in_table(inst, outgoing_state);

                for (Node succ : control_flow->successors(inst)) {
                    State succ_state = cp_analysis->get_state_in_table(succ);
                    State new_succ_state = cp_analysis->join(
                        outgoing_state, succ_state, cp_join_fn);

                    if (!state_equals(new_succ_state, succ_state)) {
                        cp_analysis->set_state_in_table(succ, new_succ_state);
                        worklist.push_back(succ);
                    }
                }
            }

            control_flow->log_instructions();
			// control_flow->log_predecessors_and_successors();
            log_cp_state_table(instructions, cp_analysis->get_state_table());

            return 0;
        });
        return constant_propagation;
    }
}
