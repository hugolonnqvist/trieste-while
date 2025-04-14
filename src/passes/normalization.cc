#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef normalization() {
        PassDef normalization = {
            "normalization",
            normalization_wf,
            dir::topdown,
            {
                In(Program) * T(Stmt)[Stmt] >> [](Match &_) -> Node {
                    auto stmt = _(Stmt);
                    if ((stmt / Stmt) != Semi) {
                        return Instructions
                            << (Normalize << (Stmt << (Semi << stmt)));
                    }
                    return Instructions << (Normalize << stmt);
                },

                T(Normalize)
                        << (T(Stmt) << T(
                                Semi, Assign, If, While, Output, Skip)[Stmt]) >>
                    [](Match &_) -> Node {
                    return Stmt << (Normalize << _(Stmt));
                },

                T(Normalize) << T(Semi)[Semi] >> [](Match &_) -> Node {
                    Nodes res = Nodes();
                    for (auto child : *_(Semi)) {
                        res.push_back(Stmt << (Normalize << child / Stmt));
                    }
                    return Semi << res;
                },

                T(Normalize)
                        << (T(Assign)
                            << (T(Ident)[Ident] *
                                (T(AExpr) << T(Add, Sub, Mul)[Op]))) >>
                    [](Match &_) -> Node {
                    Node op = _(Op);
                    auto curr = op->front();

                    for (auto it = op->begin() + 1; it != op->end(); it++) {
                        curr = AExpr << (op->type() << curr << *it);
                    }

                    auto lhs = (curr / Expr)->front();
                    auto rhs = (curr / Expr)->back();

                    return Assign << _(Ident)
                                  << (AExpr
                                      << (op->type() << (Normalize << lhs)
                                                     << (Normalize << rhs)));
                },

                T(Normalize)
                        << (T(Assign)
                            << (T(Ident)[Ident] *
                                (T(AExpr)[AExpr] << T(Int, Ident, Input)))) >>
                    [](Match &_) -> Node {
                    return Assign << _(Ident)
                                  << (AExpr << (Normalize << _(AExpr)));
                },

                T(Normalize) << (T(AExpr)[AExpr] << T(Add, Sub, Mul)[Op]) >>
                    [](Match &_) -> Node {
                    auto id = Ident ^ _(Op)->fresh();
                    auto lhs = _(Op) / Lhs;
                    auto rhs = _(Op) / Rhs;

                    auto assign = Assign
                        << id
                        << (AExpr
                            << (_(Op)->type()
                                << (Normalize << lhs) << (Normalize << rhs)));

                    return Seq << (Lift << Semi << (Stmt << assign))
                               << (Atom << id->clone());
                },

                T(Normalize) << (T(AExpr) << T(Int, Ident, Input)[Expr]) >>
                    [](Match &_) -> Node { return Atom << _(Expr); },

                T(Normalize) << T(While)[While] >> [](Match &_) -> Node {
                    auto bexpr = _(While) / BExpr;
                    auto do_stmt = _(While) / Do;
                    return While << (Normalize << bexpr)
                                 << (Normalize << do_stmt);
                },

                T(Normalize) << (T(Output) << T(AExpr)[AExpr]) >> [](Match &_)
                    -> Node { return Output << (Normalize << _(AExpr)); },

                T(Normalize) << T(If)[If] >> [](Match &_) -> Node {
                    auto bexpr = _(If) / BExpr;
                    auto then_stmt = _(If) / Then;
                    auto else_stmt = _(If) / Else;

                    return If << (Normalize << bexpr)
                              << (Normalize << then_stmt)
                              << (Normalize << else_stmt);
                },

                T(Normalize) << T(Skip)[Skip] >>
                    [](Match &_) -> Node { return _(Skip); },

                // BExpr
                T(Normalize) << (T(BExpr) << T(LT, Equals)[Op]) >>
                    [](Match &_) -> Node {
                    auto op_type = _(Op)->type();
                    Node lhs = _(Op) / Lhs;
                    Node rhs = _(Op) / Rhs;
                    return BExpr
                        << (op_type << (Normalize << lhs)
                                    << (Normalize << rhs));
                },

                T(Normalize) << (T(BExpr)[BExpr] << T(And, Or)[Op]) >>
                    [](Match &_) -> Node {
                    Nodes res = Nodes();
                    for (auto child : *_(Op)) {
                        res.push_back(Normalize << child);
                    }
                    return BExpr << (_(Op)->type() << res);
                },

                T(Normalize) << (T(BExpr)[BExpr] << T(Not)[Op]) >>
                    [](Match &_) -> Node {
                    auto expr = _(Op) / Expr;
                    return BExpr << (_(Op)->type() << (Normalize << expr));
                },

                T(Normalize) << T(BExpr)[BExpr] >>
                    [](Match &_) -> Node { return _(BExpr); },
            }};

        normalization.post([](Node n) {
            // Removes the instructions node
            auto program = n / Program;
            if (program->front() == Instructions) {
                auto inst = program->front();
                auto statement = inst->front();

                program->pop_back();
                program->push_front(statement);
            }

            return 0;
        });
        return normalization;
    }
}
