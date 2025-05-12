#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Rewriter final_rewriter() {
        Rewriter rewriter = {
            "final_rewriter",
            {
                mermaid(normalization_wf),
            },
            whilelang::normalization_wf,
        };

        return rewriter;
    }
}
