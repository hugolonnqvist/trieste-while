#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Rewriter optimization_analysis(bool run_zero_analysis) {
        auto cfg = std::make_shared<ControlFlow>();
        auto cfg_is_dirty = [=](Node) { return cfg->is_dirty(); };
        auto run_zero = [=](Node) { return run_zero_analysis; };

        Rewriter rewriter = {
            "optimization_analysis",
            {
                gather_functions(cfg),
                gather_instructions(cfg),
                gather_flow_graph(cfg),

                z_analysis(cfg).cond(run_zero),
                constant_folding(cfg),

                gather_functions(cfg).cond(cfg_is_dirty),
                gather_instructions(cfg).cond(cfg_is_dirty),
                gather_flow_graph(cfg).cond(cfg_is_dirty),

                dead_code_elimination(cfg),
                dead_code_cleanup(),
            },
            whilelang::normalization_wf,
        };

        return rewriter;
    }
}
