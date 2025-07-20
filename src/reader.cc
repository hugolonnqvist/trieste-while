#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Reader reader(
        std::shared_ptr<std::map<std::string, std::string>> vars_map,
        bool,
        bool run_mermaid) {
        auto mermaid_cond = [=](Node) { return run_mermaid; };
        return {
            "while",
            {
                // Parsing
                generate_mermaid(parse_wf).cond(mermaid_cond),
                functions(),
                generate_mermaid(functions_wf).cond(mermaid_cond),
                expressions(),
                generate_mermaid(expressions_wf).cond(mermaid_cond),
                statements(),
                generate_mermaid(statements_wf).cond(mermaid_cond),

                // Checking
                check_refs(),

                // Fix unique variables
                unique_variables(vars_map),

                // Normalization
                normalization(),
                generate_mermaid(normalization_wf).cond(mermaid_cond),

                // Used for perfomance analysis
                //gather_stats().cond([=](Node) { return run_stats; }),
            },
            whilelang::parser(),
        };
    }
}
