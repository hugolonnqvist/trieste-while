#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef statements() {
        return {
            "statements",
            statements_wf,
            dir::bottomup | dir::once,
            {
                T(Skip)[Skip] >>
                    [](Match &_) -> Node { return Stmt << _(Skip); },

                T(Var) << (T(AExpr) << T(Ident)[Ident]) >>
                    [](Match &_) -> Node { return Stmt << (Var << _(Ident)); },

                T(Assign) << ((T(AExpr) << T(Ident)[Ident]) * T(AExpr)[Rhs]) >>
                    [](Match &_) -> Node {
                    return Stmt << (Assign << _(Ident) << _(Rhs));
                },

                (T(If) << T(BExpr)[BExpr]) * (T(Then) << T(Stmt)[Then]) *
                        (T(Else) << T(Stmt)[Else]) >>
                    [](Match &_) -> Node {
                    auto then = _(Then)->front() == Block ?
                        _(Then) :
                        Stmt << (Block << _(Then));
                    auto else_ = _(Else)->front() == Block ?
                        _(Else) :
                        Stmt << (Block << _(Else));
                    return Stmt << (If << _(BExpr) << then << else_);
                },

                (T(While) << T(BExpr)[BExpr]) * (T(Do) << T(Stmt)[Do]) >>
                    [](Match &_) -> Node {
                    auto do_ = _(Do)->front() == Block ?
                        _(Do) :
                        Stmt << (Block << _(Do));
                    return Stmt << (While << _(BExpr) << do_);
                },

                T(Output) << T(AExpr)[AExpr] >> [](Match &_) -> Node {
                    return Stmt << (Output << _(AExpr));
                },

                T(FunDef)
                        << (T(FunId)[FunId] * T(ParamList)[ParamList] *
                            (T(Body) << T(Stmt)[Body])) >>
                    [](Match &_) -> Node {
                    auto body = _(Body)->front() == Block ?
                        _(Body) :
                        Stmt << (Block << _(Body));
                    return FunDef << _(FunId) << _(ParamList) << body;
                },

                T(Return)[Return] << T(AExpr) >>
                    [](Match &_) -> Node { return Stmt << _(Return); },

                T(Semi)[Semi] << T(Stmt) >> [](Match &_) -> Node {
                    return Stmt << (Block << *_(Semi));
                },

                T(Group) << (T(Stmt)[Stmt] * End) >>
                    [](Match &_) -> Node { return _(Stmt); },

                T(Brace) << (T(Stmt)[Stmt] * End) >>
                    [](Match &_) -> Node { return _(Stmt); },

                // Error rules
                T(Var)[Var] << Any >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Var))
                                 << (ErrorMsg ^
                                     "Invalid variable declaration, expected "
                                     "an identifier");
                },

                T(Group) << (T(Stmt, Brace) * T(Stmt, Brace)[Stmt]) >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Stmt))
                                 << (ErrorMsg ^ "Unexpected statement");
                },

                T(Assign) << ((T(AExpr) << T(Ident)) * Any[Rhs]) >>
                    [](Match &_) -> Node {
                    return Error
                        << (ErrorAst << _(Rhs))
                        << (ErrorMsg ^ "Invalid right-hand side to assignment");
                },

                T(Assign)[Assign]
                        << ((T(AExpr) << T(Ident)) * (T(Group) << End)) >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Assign))
                                 << (ErrorMsg ^
                                     "Expected right-hand side to assignment");
                },

                T(Assign)[Assign] << Any >> [](Match &_) -> Node {
                    return Error
                        << (ErrorAst << _(Assign))
                        << (ErrorMsg ^ "Invalid left-hand side to assignment");
                },

                T(If) * (!T(Then))[Stmt] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Stmt))
                                 << (ErrorMsg ^ "Expected 'then'");
                },

                T(Then) * (!T(Else))[Stmt] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Stmt))
                                 << (ErrorMsg ^ "Expected 'else'");
                },

                T(While) * (!T(Do))[Stmt] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Stmt))
                                 << (ErrorMsg ^ "Expected 'do'");
                },

                In(If) * Start * (!T(BExpr))[BExpr] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(BExpr))
                                 << (ErrorMsg ^ "Expected condition");
                },

                T(If)[If] << End >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(If))
                                 << (ErrorMsg ^ "Expected condition");
                },

                In(While) * Start * (!T(BExpr))[BExpr] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(BExpr))
                                 << (ErrorMsg ^ "Expected condition");
                },

                T(While)[While] << End >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(While))
                                 << (ErrorMsg ^ "Expected condition");
                },

                In(Then) * Start * (!T(Stmt))[Stmt] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Stmt))
                                 << (ErrorMsg ^ "Expected statement");
                },

                In(Else) * Start * (!T(Stmt))[Stmt] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Stmt))
                                 << (ErrorMsg ^ "Expected statement");
                },

                In(Do) * Start * (!T(Stmt))[Stmt] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Stmt))
                                 << (ErrorMsg ^ "Expected statement");
                },

                In(Output) * Start * (!T(AExpr))[AExpr] >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(AExpr))
                                 << (ErrorMsg ^ "Expected expression");
                },

                T(Body)[Body] << (Start * End) >> [](Match &_) -> Node {
                    return Error
                        << (ErrorAst << _(Body))
                        << (ErrorMsg ^ "Expected non empty function body");
                },

                T(Body)[Body] << !T(Stmt) >> [](Match &_) -> Node {
                    return Error
                        << (ErrorAst << _(Body))
                        << (ErrorMsg ^
                            "Expected the function body to be a statement");
                },

                T(ArgList)[ArgList] << !T(Arg) >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(ArgList))
                                 << (ErrorMsg ^
                                     "Expected the function arguments to be "
                                     "arguments");
                },

                T(Arg)[Arg] << !T(AExpr) >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(ArgList))
                                 << (ErrorMsg ^
                                     "Expected the function arguments to be "
                                     "arithmetic expressions");
                },

                In(Return) * Start * (!T(AExpr))[AExpr] >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(AExpr))
                                 << (ErrorMsg ^ "Expected expression");
                },

                In(Semi)[Semi] * (!T(Stmt))[Expr] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Expr))
                                 << (ErrorMsg ^ "Expected statement");
                },

                In(File) * (!T(Stmt))[Expr] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Expr))
                                 << (ErrorMsg ^ "Expected statement");
                },

                T(File) << End >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(File))
                                 << (ErrorMsg ^ "Expected statement");
                },

            }};
    }
}
