#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Reader reader(
        std::shared_ptr<std::map<std::string, std::string>> vars_map,
        bool run_stats) {
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

                // Used for perfomance analysis
                gather_stats().cond([=](Node) { return run_stats; }),
            },
            whilelang::parser(),
        };
    }
}
