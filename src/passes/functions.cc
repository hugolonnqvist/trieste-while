#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef functions() {
        PassDef functions = {
            "functions",
            functions_wf,
            dir::topdown | dir::once,
            {
                In(Top) * T(File)[File] >> [](Match &_) -> Node {
                    return Reapply << (Program << *_(File));
                },

                T(Program) << T(Semi)[Semi] >> [](Match &_) -> Node {
                    return Reapply << (Program << *_(Semi));
                },

                T(Group)
                        << (T(FunDec) * T(Ident)[Ident] *
                            (T(Paren) << T(Group)[Group]) * T(Brace)[Brace]) >>
                    [](Match &_) -> Node {
                    return FunDec << (FunId << _(Ident))
                                  << (ParamList << *_(Group))
                                  << (Body << *_(Brace));
                },

                T(ParamList)[ParamList] >> [](Match &_) -> Node {
                    Node params = ParamList;
                    for (auto ident : *_(ParamList)) {
                        params << (Param << ident);
                    }

                    return params;
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
