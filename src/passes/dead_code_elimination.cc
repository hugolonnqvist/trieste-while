#include "../analyses/dataflow_analysis.hh"
#include "../analyses/liveness.hh"
#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    std::optional<bool> get_batom_value(Node bexpr) {
        auto expr = bexpr / Expr;

        if (expr->type().in({True, False})) {
            return expr == True ? true : false;
        }

        return std::nullopt;
    };

    Node bool_to_bexpr(bool v) { return BAtom << (v ? True : False); };

    PassDef dead_code_elimination(std::shared_ptr<ControlFlow> cfg) {
        auto analysis = std::make_shared<
            DataFlowAnalysis<LiveState, std::string, LiveImpl>>();

        PassDef dead_code_elimination =
            {
                "dead_code_elimination",
                normalization_wf,
                dir::bottomup | dir::once,
                {
                    // Remove unused functions
                    T(FunDef)[FunDef] >> [=](Match &_) -> Node {
                        auto fun_id = _(FunDef) / FunId;

                        if (get_identifier(fun_id) != "main" &&
                            cfg->get_fun_calls_from_def(_(FunDef)).empty()) {
                            return {};
                        }
                        return NoChange;
                    },

                    // Remove unused variables
                    T(Stmt)
                            << (T(Assign)[Assign]
                                << (T(Ident)[Ident] * T(AExpr, BExpr))) >>
                        [=](Match &_) -> Node {
                        auto id = get_identifier(_(Ident));
                        auto assign = _(Assign);

                        if (analysis->get_state(assign).contains(id)) {
                            return NoChange;
                        } else {
                            return {};
                        }
                    },

                    // Remove empty blocks
                    T(Stmt)[Stmt] << (T(Block)[Block] << End) >>
                        [](Match &_) -> Node {
                        if (_(Stmt)->parent()->in({If, While, FunDef})) {
                            // Make sure fun defs, if and while statements
                            // don't have their body removed
                            return Stmt << (Block << (Stmt << Skip));
                        }
                        return {};
                    },

                    // Remove sequences of skip statements
                    In(Block) *
                            ((Any[Stmt] * (T(Stmt) << T(Skip))) /
                             ((T(Stmt) << T(Skip)) * Any[Stmt])) >>
                        [](Match &_) -> Node { return Reapply << _(Stmt); },

                    // Try to evaluate relational expressions
                    In(BExpr) * T(LT, Equals)[Op] >> [=](Match &_) -> Node {
                        auto op = _(Op);

                        auto lhs = (op / Lhs) / Expr;
                        auto rhs = (op / Rhs) / Expr;

                        if (lhs == Int && rhs == Int) {
                            if (op == LT) {
                                return bool_to_bexpr(
                                    get_int_value(lhs) < get_int_value(rhs));
                            } else {
                                return bool_to_bexpr(
                                    get_int_value(lhs) == get_int_value(rhs));
                            }
                        }

                        return NoChange;
                    },

                    // Try to determine branching of if statements
                    T(Stmt)
                            << (T(If)
                                << (T(BAtom)[BAtom] * T(Stmt)[Then] *
                                    T(Stmt)[Else])) >>
                        [=](Match &_) -> Node {
                        auto batom = _(BAtom);
                        auto bexpr_value = get_batom_value(batom);

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

                    // Try to determine branching of while statements
                    T(Stmt) << (T(While) << (T(Stmt) * T(BAtom)[BAtom])) >>
                        [=](Match &_) -> Node {
                        auto batom = _(BAtom);
                        auto bexpr_value = get_batom_value(batom);

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
            LiveState first_state = {};

            analysis->backward_worklist_algoritm(cfg, first_state);

            // cfg->log_instructions();
            // analysis->log_state_table(cfg);

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
