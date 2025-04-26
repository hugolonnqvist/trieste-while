#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Rewriter optimization_analysis(bool run_zero_analysis) {
        auto control_flow = std::make_shared<ControlFlow>();
        auto control_is_dirty = [=](Node) { return false; };
        auto should_run_zero_analysis = [=](Node) { return run_zero_analysis; };

        Rewriter rewriter = {
            "optimization_analysis",
            {
                gather_instructions(control_flow),
                gather_flow_graph(control_flow),
                z_analysis(control_flow).cond(should_run_zero_analysis),
                constant_folding(control_flow),

                gather_instructions(control_flow).cond(control_is_dirty),
                gather_flow_graph(control_flow).cond(control_is_dirty),
                dead_code_elimination(control_flow),
                dead_code_cleanup(),
            },
            whilelang::normalization_wf,
        };

        return rewriter;
    }
}
