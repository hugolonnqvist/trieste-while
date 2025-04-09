#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Rewriter optimization_analysis() {
        auto control_flow = std::make_shared<ControlFlow>();

		Rewriter rewriter = {
            "optimization_analysis",
            {
                gather_instructions(control_flow),
                gather_flow_graph(control_flow),
                constant_folding(control_flow),

                gather_instructions(control_flow),
                gather_flow_graph(control_flow),
                dead_code_elimination(control_flow),
                // z_analysis(control_flow),
            },
            whilelang::normalization_wf,
        };

		return rewriter;
    }
}
