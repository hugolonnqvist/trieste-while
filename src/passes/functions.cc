#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef functions() {
        PassDef functions = {
            "functions",
            functions_wf,
            dir::topdown,
            {
                In(Top) * (T(File)[File] << (T(FunDef) * T(FunDef)++)) >>
                    [](Match &_) -> Node { return Program << *_(File); },

                T(File) << T(Semi)[Semi] >>
                    [](Match &_) -> Node { return File << *_(Semi); },

                T(Semi)[Semi] << (T(FunDef) * T(FunDef)++) >>
                    [](Match &_) -> Node { return Seq << *_(Semi); },

                T(Var) * T(Ident)[Ident] >>
                    [](Match &_) -> Node { return Var << _(Ident); },

                T(Group)
                        << (T(FunDef) * T(Ident)[Ident] * T(Paren)[Paren] *
                            T(Brace)[Brace]) >>
                    [](Match &_) -> Node {
                    return FunDef << (FunId << _(Ident))
                                  << (ParamList << *_(Paren))
                                  << (Body << *_(Brace));
                },

                T(ParamList) << (Start * End) >>
                    [](Match &) -> Node { return NoChange; },

                T(ParamList) << T(Group)[Group] >> [](Match &_) -> Node {
                    Node params = ParamList;
                    for (auto ident : *_(Group)) {
                        params << (Param << ident);
                    }

                    return params;
                },

                T(Ident)[Ident] * T(Paren)[Paren] >> [](Match &_) -> Node {
                    return FunCall << (FunId << _(Ident))
                                   << (ArgList << *_(Paren));
                },

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

                T(Program)[Program] << !T(FunDef) >> [](Match &_) -> Node {
                    return Error
                        << (ErrorAst << _(Program))
                        << (ErrorMsg ^ "Invalid program, missing a function");
                },

                T(Var)[Var] << !T(Ident) >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Var))
                                 << (ErrorMsg ^
                                     "Invalid variable declaration, expected "
                                     "an identifier");
                },

            }};

        functions.pre([](Node n) {
            logging::Debug() << n;
            return 0;
        });

        return functions;
    }
}
