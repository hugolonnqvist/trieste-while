#include "../internal.hh"
namespace whilelang {

    using namespace trieste;

    PassDef gather_vars() {
        return {
            "gather_vars",
            gather_vars_wf,
            dir::topdown | dir::once,
            {
                T(FunDef)[FunDef] >>
                    [](Match &_) -> Node {
                        auto fun_id = _(FunDef) / FunId;
                        auto param_list = _(FunDef) / ParamList;
                        auto body = _(FunDef) / Body;

                        Nodes vars;
                        (body / Stmt)->get_symbols(vars, [](const Node &n) {
                            return n == Var;
                        });

                        Node idents = Idents;
                        for (const auto &var : vars) {
                            idents << (var / Ident);
                        }

                        return FunDef << fun_id << param_list << idents << body;
                    },

                T(Stmt) << T(Var) >>
                    [](Match &_) -> Node {
                        return Stmt << Skip;
                    },
            }};
    }
}
