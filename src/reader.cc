#include "control_flow.hh"
#include "internal.hh"
#include "utils.hh"

namespace whilelang {
    using namespace trieste;

    Reader
    reader(std::shared_ptr<std::map<std::string, std::string>> vars_map) {
        return {
            "while",
            {
                // Parsing
                functions(),
                expressions(),
                statements(),

                // Checking
                check_refs(),

                // Fix unique variables
                unique_variables(vars_map),

                // Normalization
                normalization(),
            },
            whilelang::parser(),
        };
    }
}
