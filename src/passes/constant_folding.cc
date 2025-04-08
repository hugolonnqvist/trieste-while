#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    StateValue atom_flow(Node inst, State incoming_state) {
        if (inst == Atom) {
            Node expr = inst / Expr;

            if (expr == Int) {
                return {TConstant, get_int_value(expr)};
            } else if (expr == Ident) {
                std::string rhs_ident = get_identifier(expr);
                return incoming_state[rhs_ident];
            }
        }

        return {TTop, 0};
    }

    State cp_flow_fn(Node inst, State incoming_state) {
        if (inst == Assign) {
            std::string ident = get_identifier(inst / Ident);

            auto rhs_expr = (inst / Rhs) / Expr;
            if (rhs_expr == Atom) {
                incoming_state[ident] = atom_flow(rhs_expr, incoming_state);
            } else {
                Node lhs = rhs_expr / Lhs;
                Node rhs = rhs_expr / Rhs;

                StateValue lhs_st = atom_flow(lhs, incoming_state);
                StateValue rhs_st = atom_flow(rhs, incoming_state);

                if (lhs_st.type == TConstant && rhs_st.type == TConstant) {
                    int calculated_value;
                    if (rhs_expr == Add) {
                        calculated_value = lhs_st.value + rhs_st.value;
                    } else if (rhs_expr == Sub) {
                        calculated_value = lhs_st.value - rhs_st.value;
                    } else {
                        calculated_value = lhs_st.value * rhs_st.value;
                    }

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

    PassDef constant_folding(std::shared_ptr<ControlFlow> control_flow,
                             bool& changed) {
        auto cp_analysis = std::make_shared<DataFlowAnalysis>();

        auto ident_to_const = [cp_analysis, &changed](int value) -> Node {
            auto constant = create_const(value);
            changed = true;

            return Atom << constant;
        };

        auto try_atom_to_const = [cp_analysis, ident_to_const](
                                     Node inst, Node atom) -> Node {
            if ((atom / Expr) == Ident) {
                auto state_value =
                    cp_analysis->get_state_value(inst, atom / Expr);

                if (state_value.type == TConstant) {
                    return ident_to_const(state_value.value);
                }
            }
            return atom;
        };

        // clang-format off
        PassDef constant_folding =  {
            "constant_folding",
            normalization_wf,
            dir::bottomup | dir::once,
            {
				T(Assign)[Assign] << (T(Ident)[Ident] * (T(AExpr) << T(Add, Sub, Mul))) >>
					[cp_analysis, ident_to_const](Match &_) -> Node 
					{
						auto inst = _(Assign);
						auto ident = _(Ident);
						
						auto state_value = cp_analysis->get_state_value(inst, ident);
						
						if (state_value.type == TConstant) {
							auto constant = ident_to_const(state_value.value);
							return Assign << ident
										  << (AExpr << constant);
						}

						return NoChange;
					},

				T(Output)[Output] << (T(Atom)[Atom] << T(Ident)) >>
					[cp_analysis, try_atom_to_const](Match &_) -> Node 
					{
						auto inst = _(Output);

						return Output << try_atom_to_const(inst, _(Atom));
					},

				T(BExpr)[BExpr] << (T(LT, Equals)[Op] << (T(Atom)[Lhs] * T(Atom)[Rhs]))>>
					[cp_analysis, try_atom_to_const](Match &_) -> Node 
					{
						auto inst = _(BExpr);

						return BExpr << (_(Op)->type() << try_atom_to_const(inst, _(Lhs))	
													   << try_atom_to_const(inst, _(Rhs)));
				
					},
			}
		};

        // clang-format on
        constant_folding.pre([=](Node) {
            const auto instructions = control_flow->get_instructions();
            const Vars vars = control_flow->get_vars();
            cp_analysis->init(instructions, vars);

            std::deque<Node> worklist{instructions[0]};

            while (!worklist.empty()) {
                Node inst = worklist.front();
                worklist.pop_front();

                State in_state = cp_analysis->get_state_in_table(inst);
                State out_state = cp_flow_fn(inst, in_state);

                cp_analysis->set_state_in_table(inst, out_state);

                for (Node succ : control_flow->successors(inst)) {
                    State succ_state = cp_analysis->get_state_in_table(succ);
                    State new_succ_state =
                        cp_analysis->join(out_state, succ_state, cp_join_fn);

                    if (!state_equals(new_succ_state, succ_state)) {
                        cp_analysis->set_state_in_table(succ, new_succ_state);
                        worklist.push_back(succ);
                    }
                }
            }

            control_flow->log_instructions();
            log_cp_state_table(instructions, cp_analysis->get_state_table());

            return 0;
        });

        return constant_folding;
    }
}
