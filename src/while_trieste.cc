#include "internal.hh"

#include <trieste/driver.h>

int main(int argc, char **argv) {
    using namespace whilelang;
    using namespace trieste;
    auto vars_map = std::make_shared<std::map<std::string, std::string>>();
    return Driver({
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

                // Compilation
                to3addr(),
                gather_vars(),
                blockify(),
                compile(),
            },
            parser(),
        })
        .run(argc, argv);
}
