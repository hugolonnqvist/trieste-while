#include "control_flow.hh"
#include "internal.hh"
#include "utils.hh"

namespace whilelang {
    using namespace trieste;

    Reader reader() {
		auto control_flow = std::make_shared<ControlFlow>();

        return {
            "while",
            {
                // Parsing
                expressions(),
                statements(),

                // Checking
                check_refs(),

                // Normalization
                normalization(),

                // Static analysis
                gather_instructions(control_flow),
                // gather_flow_graph(control_flow),

                // z_analysis(control_flow),
                constant_propagation(control_flow),

            },
            whilelang::parser(),
        };
    }
}
