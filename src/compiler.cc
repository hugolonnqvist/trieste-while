#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Rewriter compiler() {
        return {
            "compiler",
            {
                to3addr(),
                gather_vars(),
                blockify(),
                compile(),
            },
            whilelang::normalization_wf
        };
    }
}
