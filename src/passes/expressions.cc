#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef expressions() {
        auto UNHANDLED = --In(BExpr, AExpr, Param);
        return {
            "expressions",
            expressions_wf,
            dir::bottomup,
            {

                T(Ident)[Ident] * T(Paren)[Paren] >> [](Match &_) -> Node {
                    return AExpr
                        << (FunCall << (FunId ^ _(Ident))
                                    << (ArgList << *_(Paren)));
                },

                T(ArgList)
                        << (T(Comma, Group)[Group]
                            << --(T(Ident) * T(Paren))) >>
                    [](Match &_) -> Node {
                    Node args = ArgList;
                    for (auto child : *_(Group)) {
                        args << (Arg << child);
                    }

                    return args;
                },

                In(ArgList) * T(AExpr)[AExpr] >>
                    [](Match &_) -> Node { return Arg << _(AExpr); },

                UNHANDLED * T(True, False)[Expr] >>
                    [](Match &_) -> Node { return BExpr << _(Expr); },

                UNHANDLED * T(Ident, Int, Input, FunCall)[Expr] >>
                    [](Match &_) -> Node { return AExpr << _(Expr); },

                UNHANDLED * (T(Not) << End) * T(BExpr)[BExpr] >>
                    [](Match &_) -> Node { return BExpr << (Not << _(BExpr)); },

                UNHANDLED * T(Add, Sub, Mul)[Op] << (T(AExpr) * T(AExpr)++) >>
                    [](Match &_) -> Node { return AExpr << _(Op); },

                UNHANDLED * T(Equals, LT)[Op] << (T(AExpr) * T(AExpr) * End) >>
                    [](Match &_) -> Node { return BExpr << _(Op); },

                UNHANDLED * T(And, Or)[Op] << (T(BExpr) * T(BExpr)) >>
                    [](Match &_) -> Node { return BExpr << _(Op); },

                UNHANDLED * T(Paren) << (T(AExpr, BExpr)[Expr] * End) >>
                    [](Match &_) -> Node { return _(Expr); },

                UNHANDLED * T(Group) << (T(AExpr, BExpr)[Expr] * End) >>
                    [](Match &_) -> Node { return _(Expr); },

                // Error rules
                T(Paren)[Paren] << End >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Paren))
                                 << (ErrorMsg ^ "Empty parenthesis");
                },

                In(Group) * ((--Start * T(AExpr, BExpr)[Expr])) >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Expr))
                                 << (ErrorMsg ^ "Unexpected expression");
                },

                In(Group) * T(AExpr, BExpr) * Any[Rhs] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Rhs))
                                 << (ErrorMsg ^
                                     ((std::string) "Unexpected " +
                                      _(Rhs)->type().str()));
                },

                (T(Not)[Not] << End) * End >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Not))
                                 << (ErrorMsg ^ "Not enough operands");
                },

                T(Not)[Not] * (!T(True, False, Paren))[Expr] >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Expr))
                                 << (ErrorMsg ^ "Invalid operand");
                },

                T(Add, Sub, Mul, Equals, LT)[Op] << (T(AExpr, Paren) * End) >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Op))
                                 << (ErrorMsg ^ "Not enough operands");
                },

                In(Add, Sub, Mul, Equals, LT) * (T(Group) << End)[Expr] >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Expr))
                                 << (ErrorMsg ^ "Expected operand");
                },

                In(Add, Sub, Mul, Equals, LT) * (!T(AExpr, Paren))[Expr] >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Expr))
                                 << (ErrorMsg ^ "Invalid operand");
                },

                T(And, Or)[Op] << (T(BExpr, Paren) * End) >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Op))
                                 << (ErrorMsg ^ "Not enough operands");
                },

                In(And, Or) * (T(Group) << End)[Expr] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Expr))
                                 << (ErrorMsg ^ "Expected operand");
                },

                In(And, Or) * (!T(BExpr))[Expr] >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Expr))
                                 << (ErrorMsg ^ "Invalid operand");
                },

                T(Arg)[Arg] << (T(Group) << (Start * End)) >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Arg))
                                 << (ErrorMsg ^ "Invalid empty argument");
                },
            }};
    }
}
