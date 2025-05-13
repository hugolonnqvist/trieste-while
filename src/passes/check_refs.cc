#include "../internal.hh"
namespace whilelang {

    using namespace trieste;

    PassDef check_refs() {
        return {
            "check_refs",
            statements_wf,
            dir::bottomup | dir::once,
            {
                T(AExpr, Assign) << T(Ident)[Ident] >> [](Match &_) -> Node {
                    auto def = _(Ident)->lookup();
                    if (def.empty()) {
                        return Error << (ErrorAst << _(Ident))
                                     << (ErrorMsg ^ "Undefined variable");
                    } else if (def.size() > 1) {
                        return Error
                            << (ErrorAst << _(Ident))
                            << (ErrorMsg ^
                                "Variable can not be defined multiple times");
                    }

                    return NoChange;
                },
            }};
    }
}
