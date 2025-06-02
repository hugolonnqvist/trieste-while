#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef normalization() {
        PassDef normalization = {
            "normalization",
            normalization_wf,
            dir::topdown,
            {
                // Error needed for fuzz testing,
                // otherwise lift with no destination error
                T(Normalize)
                        << (T(FunDef)[FunDef]
                            << (T(FunId) * T(ParamList) *
                                (T(Stmt)[Body] << --T(Block)))) >>
                    [](Match &_) -> Node {
                    return Error
                        << (ErrorAst << _(FunDef))
                        << (ErrorMsg ^
                            "Expected the function body to be a block stmt");
                },

                T(Program)[Program] << T(FunDef) >> [](Match &_) -> Node {
                    Node res = Instructions;
                    for (auto child : *_(Program)) {
                        res << (Normalize << child);
                    }
                    return Program << res;
                },
                T(Normalize) << T(FunDef)[FunDef] >> [](Match &_) -> Node {
                    return FunDef << (_(FunDef) / FunId)
                                  << (_(FunDef) / ParamList)
                                  << (Normalize << (_(FunDef) / Body));
                },

                T(Normalize)
                        << (T(Stmt)
                            << T(Block, Assign, If, While, Output, Skip, Return, Var)
                                   [Stmt]) >>
                    [](Match &_) -> Node {
                    return Stmt << (Normalize << _(Stmt));
                },

                T(Normalize) << T(Block)[Block] >> [](Match &_) -> Node {
                    Node res = Block;
                    for (auto child : *_(Block)) {
                        res << (Stmt << (Normalize << child / Stmt));
                    }
                    return res;
                },

                T(Stmt) << (T(Normalize) << T(Var)) >>
                    [](Match &) -> Node { return Stmt << Skip; },

                T(Normalize)
                        << (T(Assign)
                            << (T(Ident)[Ident] *
                                (T(AExpr)[AExpr] << T(Add, Sub, Mul)[Op]))) >>
                    [](Match &_) -> Node {
                    Node op = _(Op);
                    auto curr = op->front();

                    // Make sure binary ops have exactly two operands
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

                T(Normalize)
                        << (T(Assign)
                            << (T(Ident)[Ident] *
                                (T(AExpr)[AExpr] << T(FunCall)[FunCall]))) >>
                    [](Match &_) -> Node {
                    auto fun_id = _(FunCall) / FunId;
                    Node args = ArgList;

                    for (auto child : *(_(FunCall) / ArgList)) {
                        args << (Normalize << child);
                    }

                    return Assign << _(Ident)
                                  << (AExpr << (FunCall << fun_id << args));
                },

                T(Normalize) << (T(AExpr) << T(FunCall)[FunCall]) >>
                    [](Match &_) -> Node {
                    auto id = Ident ^ _(FunCall)->fresh();
                    auto fun_id = _(FunCall) / FunId;
                    Node args = ArgList;

                    for (auto child : *(_(FunCall) / ArgList)) {
                        args << (Normalize << child);
                    }

                    auto assign = Assign
                        << id << (AExpr << (FunCall << fun_id << args));

                    return Seq << (Lift << Block << (Stmt << assign))
                               << (Atom << id->clone());
                },

                T(Normalize) << (T(Arg) << T(AExpr)[AExpr]) >> [](Match &_)
                    -> Node { return Arg << (Normalize << _(AExpr)); },

                T(Normalize) << (T(AExpr)[AExpr] << T(Add, Sub, Mul)[Op]) >>
                    [](Match &_) -> Node {
                    Node op = _(Op);
                    auto id = Ident ^ _(Op)->fresh();

                    auto curr = op->front();

                    // Make sure binary ops have exactly two operands
                    for (auto it = op->begin() + 1; it != op->end(); it++) {
                        curr = AExpr << (op->type() << curr << *it);
                    }

                    auto lhs = (curr / Expr)->front();
                    auto rhs = (curr / Expr)->back();

                    auto assign = Assign
                        << id
                        << (AExpr
                            << (_(Op)->type()
                                << (Normalize << lhs) << (Normalize << rhs)));

                    return Seq << (Lift << Block << (Stmt << assign))
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

                T(Normalize) << (T(Return) << T(AExpr)[AExpr]) >> [](Match &_)
                    -> Node { return Return << (Normalize << _(AExpr)); },

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
                    Node res = _(Op)->type();
                    for (auto child : *_(Op)) {
                        res << (Normalize << child);
                    }
                    return BExpr << res;
                },

                T(Normalize) << (T(BExpr)[BExpr] << T(Not)[Op]) >>
                    [](Match &_) -> Node {
                    auto expr = _(Op) / Expr;
                    return BExpr << (_(Op)->type() << (Normalize << expr));
                },

                T(Normalize) << T(BExpr)[BExpr] >>
                    [](Match &_) -> Node { return _(BExpr); },

            }};

        normalization.post([=](Node n) {
            // Removes the instructions node
            Node program = n / Program;
            if (program->front() == Instructions) {
                Node inst = program->front();

                program->pop_back();
                program->push_back(*inst);
            }

            return 0;
        });

        return normalization;
    }
}
