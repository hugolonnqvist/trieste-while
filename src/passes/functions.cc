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

                T(FunDef)[FunDef] << --(T(FunId) * T(ParamList) * T(Body)) >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(FunDef))
                                 << (ErrorMsg ^ "Invalid function declaration");
                },

                T(Program)[Program] << !T(FunDef) >> [](Match &) -> Node {
                    return Error
                        << (ErrorAst << Program)
                        << (ErrorMsg ^ "Invalid program, missing a function");
                },

            }};

        functions.pre([](Node n) {
            std::cout << "Pre functions pass:\n" << n;
            return 0;
        });

        functions.post([](Node n) {
            std::cout << "Post functions pass:\n" << n;
            return 0;
        });
        return functions;
    }
}
