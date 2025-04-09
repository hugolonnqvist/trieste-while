#include "../control_flow.hh"
#include "../data_flow_analysis.hh"
#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    State z_flow(Node inst, State incoming_state) {
        if (inst == Assign) {
            std::string var = get_identifier(inst / Ident);
            Node rhs = (inst / Rhs) / Expr;

            if (rhs == Atom) {
                auto atom = rhs / Expr;
                if (atom == Int) {
                    incoming_state[var].type =
                        get_int_value(atom) == 0 ? TZero : TNonZero;
                } else if (atom == Ident) {
                    std::string rhs_var = get_identifier(atom);
                    incoming_state[var] = incoming_state[rhs_var];
                } else {
                    // Rhs must be input, by wf
                    incoming_state[var].type = TTop;
                }
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

    PassDef z_analysis(std::shared_ptr<ControlFlow> control_flow) {
        auto analysis = std::make_shared<DataFlowAnalysis>();

        // clang-format off
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
        z_analysis.post([=](Node) {
            analysis->forward_worklist_algoritm(control_flow, z_flow, z_join);
            control_flow->log_instructions();
            log_z_state_table(control_flow->get_instructions(),
                              analysis->get_state_table());

            return 0;
        });

        return z_analysis;
    }
}
