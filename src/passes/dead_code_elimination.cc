#include "../analyses/liveness.hh"
#include "../analyses/dataflow_analysis.hh"
#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    std::optional<bool> get_bexpr_value(Node bexpr) {
        auto expr = bexpr / Expr;
        if (expr->type().in({True, False})) {
            return expr == True ? true : false;
        } else if (expr->type().in({LT, Equals})) {
            auto lhs = (expr / Lhs) / Expr;
            auto rhs = (expr / Rhs) / Expr;

            if (lhs == Int && rhs == Int) {
                if (expr == LT) {
                    return get_int_value(lhs) < get_int_value(rhs);
                } else {
                    return get_int_value(lhs) == get_int_value(rhs);
                }
            }
            return std::nullopt;
        } else if (expr == Not) {
            return !get_bexpr_value(expr / Expr);
        } else {
            return std::nullopt;
        }
    };

    PassDef dead_code_elimination(std::shared_ptr<ControlFlow> cfg) {
        auto liveness = std::make_shared<DataFlowAnalysis<State, std::string>>(
            live_create_state, live_state_join, live_flow);

        PassDef dead_code_elimination =
            {
                "dead_code_elimination",
                normalization_wf,
                dir::bottomup | dir::once,
                {
                    T(FunDef)[FunDef] >> [=](Match &_) -> Node {
                        auto fun_id = (_(FunDef) / FunId) / Ident;

                        if (fun_id->location().view() != "main" &&
                            cfg->get_fun_calls_from_def(_(FunDef)).empty()) {
                            return {};
                        }
                        return NoChange;
                    },

                    T(Stmt)
                            << (T(Assign)[Assign]
                                << (T(Ident)[Ident] * T(AExpr)[AExpr])) >>
                        [=](Match &_) -> Node {
                        auto id = get_identifier(_(Ident));
                        auto assign = _(Assign);

                        if (liveness->get_state(assign).contains(id)) {
                            return NoChange;
                        } else {
                            return {};
                        }
                    },

                    T(Stmt)[Stmt] << (T(Block)[Block] << End) >>
                        [](Match &_) -> Node {
                        if (_(Stmt)->parent()->in({If, While, FunDef})) {
                            // Make sure fun defs and if & while statements
                            // don't have their body removed
                            return Stmt << (Block << (Stmt << Skip));
                        }
                        return {};
                    },

                    In(Block) *
                            ((Any[Stmt] * (T(Stmt) << T(Skip))) /
                             ((T(Stmt) << T(Skip)) * Any[Stmt])) >>
                        [](Match &_) -> Node { return Reapply << _(Stmt); },

                    T(Stmt)
                            << (T(If)
                                << (T(BExpr)[BExpr] * T(Stmt)[Then] *
                                    T(Stmt)[Else])) >>
                        [=](Match &_) -> Node {
                        auto bexpr = _(BExpr);
                        auto bexpr_value = get_bexpr_value(bexpr);

                        if (bexpr_value.has_value()) {
                            if (*bexpr_value) {
                                return Reapply << _(Then);
                            } else {
                                return Reapply << _(Else);
                            }
                        } else {
                            return NoChange;
                        }
                    },

                    T(Stmt) << (T(While) << (T(BExpr)[BExpr] * T(Stmt)[Do])) >>
                        [=](Match &_) -> Node {
                        auto bexpr = _(BExpr);
                        auto bexpr_value = get_bexpr_value(bexpr);

                        if (bexpr_value.has_value()) {
                            if (*bexpr_value) {
                                return NoChange;
                            } else {
                                return {};
                            }
                        } else {
                            return NoChange;
                        }
                    },

                }};

        dead_code_elimination.pre([=](Node) {
            State first_state = {};

            liveness->backward_worklist_algoritm(cfg, first_state);

            cfg->log_instructions();
            // log_liveness(cfg, liveness);

            return 0;
        });

        return dead_code_elimination;
    }

    PassDef dead_code_cleanup() {
        return {
            "dead_code_cleanup",
            normalization_wf,
            dir::topdown | dir::once,
            {
                In(Block) * T(Stmt) << T(Block)[Block] >>
                    [](Match &_) -> Node { return Seq << *_(Block); },
            }};
    }
}
