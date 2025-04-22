#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef check_refs() {
        PassDef check_refs = {
            "check_refs",
            statements_wf,
            dir::bottomup | dir::once,
            {
                T(AExpr) << T(Ident)[Ident] >> [](Match &_) -> Node {
                    auto def = _(Ident)->lookup();
                    if (def.empty()) {
                        return Error << (ErrorAst << _(Ident))
                                     << (ErrorMsg ^ "Undefined variable");
                    }
                    return NoChange;
                },
            }};

        check_refs.pre([](Node ast) {
            logging::Debug() << ast;
            return 0;
        });

        return check_refs;
    }
}
