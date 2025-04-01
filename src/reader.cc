#include "internal.hh"

namespace whilelang {
    using namespace trieste;

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
                gather_instructions(),
                init_flow_graph(),
            },
            whilelang::parser(),
        };
    }
}
