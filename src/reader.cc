#include "control_flow.hh"
#include "internal.hh"
#include "utils.hh"

namespace whilelang {
    using namespace trieste;

    Reader reader() {
        return {
            "while",
            {
                // Parsing
				functions(),
                expressions(),
                statements(),

                // Checking
                // check_refs(),

                // Normalization
                // normalization(),
            },
            whilelang::parser(),
        };
    }
}
