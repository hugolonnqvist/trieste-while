#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Rewriter statistics_rewriter(std::shared_ptr<Nodes> instructions) {
        return {
            "statistics_rewriter",
            {
                gather_stats(instructions),
            },
            whilelang::normalization_wf,
        };
    }
}
