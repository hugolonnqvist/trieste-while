#include "control_flow.hh"
#include "internal.hh"
#include "utils.hh"

namespace whilelang {
    using namespace trieste;

    auto control_flow = std::make_shared<ControlFlow>();
    Reader reader() {
        return {
            "while",
            {
                // Parsing
                expressions(),
                statements(),

                // Checking
                check_refs(),

                // Static analysis
                gather_instructions(control_flow),
                init_flow_graph(control_flow),
            },
            whilelang::parser(),
        };
    }
}
