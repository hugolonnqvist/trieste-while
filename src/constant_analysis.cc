#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Rewriter constant_analysis(bool& changed) {
        auto control_flow = std::make_shared<ControlFlow>();

		Rewriter rewriter = {
            "constant_analysis",
            {
                gather_instructions(control_flow),
                gather_flow_graph(control_flow),
                constant_folding(control_flow, changed),
            },
            whilelang::normalization_wf,
        };

		return rewriter;
    }
}
