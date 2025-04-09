#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Rewriter live_analysis(bool& changed) {
        auto control_flow = std::make_shared<ControlFlow>();

        Rewriter rewriter = {
            "live_analysis",
            {
                gather_instructions(control_flow),
                gather_flow_graph(control_flow),
                dead_code_elimination(control_flow),
            },
            whilelang::normalization_wf,
        };

        return rewriter;
    }
}
