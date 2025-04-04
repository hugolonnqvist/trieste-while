#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    // clang-format off
    PassDef constant_propagation() {
        return {
            "constant_propagation",
            statements_wf,
            dir::topdown | dir::once,
            {
                In(AExpr) * T(Ident)[Ident] >>
                    [](Match &_) -> Node
                    {
                        // auto def = _(Ident)->lookup();
                        // if (def.size() == 0)
                        // {
                        //     return Error << (ErrorAst << _(Ident))
                        //                  << (ErrorMsg ^ "Undefined variable");
                        // }
						_(Ident);
                        return NoChange;
                    },
           }};
    }
}
