#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef check_refs() {
        return {
            "check_refs",
            statements_wf,
            dir::bottomup | dir::once,
            {
                In(AExpr) * T(Ident)[Ident] >> [](Match &_) -> Node {
                    auto def = _(Ident)->lookup();
                    if (def.size() == 0) {
                        return Error << (ErrorAst << _(Ident))
                                     << (ErrorMsg ^ "Undefined variable");
                    }
                    return NoChange;
                },
            }};
    }
}
