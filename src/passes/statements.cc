#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    // clang-format off
    PassDef statements() {
        return {
            "statements",
            statements_wf,
            dir::bottomup | dir::once,
            {
				// Make sure there is always one semi stmt
                T(Program) << (T(Stmt)[Stmt] << !T(Semi)) >>
                    [](Match &_) -> Node
                    {
                        return Program << (Stmt << (Semi << _(Stmt)));
                    },

                T(File) << (T(Stmt)[Stmt] * End) >>
                    [](Match &_) -> Node
                    {
                        return Reapply << (Program << _(Stmt));
                    },

                T(Skip)[Skip] >>
                    [](Match &_) -> Node
                    {
                        return Stmt << _(Skip);
                    },

                T(Assign) << ((T(AExpr) << T(Ident)[Ident]) * T(AExpr)[Rhs]) >>
                    [](Match &_) -> Node
                    {
                        return Stmt << (Assign << _(Ident)
                                               << _(Rhs));
                    },

                (T(If) << T(BExpr)[BExpr]) * (T(Then) << T(Stmt)[Then]) * (T(Else) << T(Stmt)[Else]) >>
                    [](Match &_) -> Node
                    {
                        return Stmt << (If << _(BExpr)
                                           << _(Then)
                                           << _(Else));
                    },

                (T(While) << T(BExpr)[BExpr]) * (T(Do) << T(Stmt)[Do]) >>
                    [](Match &_) -> Node
                    {
                        return Stmt << (While << _(BExpr)
                                              << _(Do));
                    },

                (T(Output) << T(AExpr)[AExpr]) >>
                    [](Match &_) -> Node
                    {
                        return Stmt << (Output << _(AExpr));
                    },

                T(Semi)[Semi] << T(Stmt) >>
                    [](Match &_) -> Node
                    {
                        return Stmt << _(Semi);
                    },

                T(Group) << (T(Stmt)[Stmt] * End) >>
                    [](Match &_) -> Node
                    {
                        return _(Stmt);
                    },

                T(Brace) << (T(Stmt)[Stmt] * End) >>
                    [](Match &_) -> Node
                    {
                        return _(Stmt);
                    },

                // Error rules
                T(Group) << (T(Stmt, Brace) * T(Stmt, Brace)[Stmt]) >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Stmt))
                                     << (ErrorMsg ^ "Unexpected statement");
                    },

                T(Assign) << ((T(AExpr) << T(Ident)) * Any[Rhs]) >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Rhs))
                                     << (ErrorMsg ^ "Invalid right-hand side to assignment");
                    },

                T(Assign)[Assign] << ((T(AExpr) << T(Ident)) * (T(Group) << End)) >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Assign))
                                     << (ErrorMsg ^ "Expected right-hand side to assignment");
                    },

                T(Assign)[Assign] << Any >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Assign))
                                     << (ErrorMsg ^ "Invalid left-hand side to assignment");
                    },

                T(If) * (!T(Then))[Stmt] >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Stmt))
                                     << (ErrorMsg ^ "Expected 'then'");
                    },

                T(Then) * (!T(Else))[Stmt] >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Stmt))
                                     << (ErrorMsg ^ "Expected 'else'");
                    },

                T(While) * (!T(Do))[Stmt] >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Stmt))
                                     << (ErrorMsg ^ "Expected 'do'");
                    },

                In(If) * Start * (!T(BExpr))[BExpr] >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(BExpr))
                                     << (ErrorMsg ^ "Expected condition");
                    },

                T(If)[If] << End >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(If))
                                     << (ErrorMsg ^ "Expected condition");
                    },

                In(While) * Start * (!T(BExpr))[BExpr] >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(BExpr))
                                     << (ErrorMsg ^ "Expected condition");
                    },

                T(While)[While] << End >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(While))
                                     << (ErrorMsg ^ "Expected condition");
                    },

                In(Then) * Start * (!T(Stmt))[Stmt] >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Stmt))
                                     << (ErrorMsg ^ "Expected statement");
                    },

                In(Else) * Start * (!T(Stmt))[Stmt] >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Stmt))
                                     << (ErrorMsg ^ "Expected statement");
                    },

                In(Do) * Start * (!T(Stmt))[Stmt] >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Stmt))
                                     << (ErrorMsg ^ "Expected statement");
                    },

                In(Output) * Start * (!T(AExpr))[AExpr] >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(AExpr))
                                     << (ErrorMsg ^ "Expected expression");
                    },

                In(Semi)[Semi] * (!T(Stmt))[Expr] >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Expr))
                                     << (ErrorMsg ^ "Expected statement");
                    },

                In(File) * (!T(Stmt))[Expr] >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(Expr))
                                     << (ErrorMsg ^ "Expected statement");
                    },

				T(File) << End >>
                    [](Match &_) -> Node
                    {
                        return Error << (ErrorAst << _(File))
                                     << (ErrorMsg ^ "Expected statement");
                    },

            }};
    }
}
