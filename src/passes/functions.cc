#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef functions() {
        return {
            "functions",
            functions_wf,
            dir::topdown,
            {
                In(Top) * (T(File)[File] << (T(FunDef))) >>
                    [](Match &_) -> Node { return Program << *_(File); },

                T(Group)
                        << (T(FunDef) * T(Ident)[Ident] * T(Paren)[Paren] *
                            T(Brace)[Brace]) >>
                    [](Match &_) -> Node {
                    return FunDef << (FunId ^ _(Ident))
                                  << (ParamList << *_(Paren))
                                  << (Body << *_(Brace));
                },

                T(ParamList) << T(Comma, Group)[Group] >> [](Match &_) -> Node {
                    Node params = ParamList;
                    for (auto ident : *_(Group)) {
                        params << (Param << ident);
                    }

                    return params;
                },

                In(Param) * T(Group)[Group] >>
                    [](Match &_) -> Node { return Seq << *_(Group); },

                // Error rules
                T(FunDef)[FunDef] << --(T(FunId) * T(ParamList) * T(Body)) >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(FunDef))
                                 << (ErrorMsg ^ "Invalid function declaration");
                },

                T(File)[File] << (!(
                    (T(FunDef) * T(FunDef)++) / (T(Group) << T(FunDef)))) >>
                    [](Match &_) -> Node {
                    return Error
                        << (ErrorAst << _(File))
                        << (ErrorMsg ^
                            "Invalid program, missing function declaration");
                },

                T(File)[File] << (Start * End) >> [](Match &_) -> Node {
                    return Error
                        << (ErrorAst << _(File))
                        << (ErrorMsg ^
                            "Invalid program, missing function declaration");
                },

                In(Program) * (!T(FunDef))[Expr] >> [](Match &_) -> Node {
                    return Error
                        << (ErrorAst << _(Expr))
                        << (ErrorMsg ^ "Unexpected term");
                },
            }};
    }
}
