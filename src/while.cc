#include "lang.hh"

#include <CLI/CLI.hpp>
#include <trieste/trieste.h>

int main(int argc, char const *argv[]) {
    CLI::App app;

    std::filesystem::path input_path;
    app.add_option("input", input_path, "Path to the input file ")->required();

    std::string log_level;
    app.add_option(
           "-l,--log",
           log_level,
           "Set the log level (None, Error, Output, Warn, Info, Debug, Trace).")
        ->check(trieste::logging::set_log_level_from_string);

    bool run = false;
    bool constant_analysis = false;
    app.add_flag("-r,--run", run, "Run the program (prompting inputs).");
    app.add_flag(
        "-c,--constant-analysis",
        constant_analysis,
        "Compile and run constant analysis on the program.");

    // trieste::logging::set_log_level_from_string("Debug");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    auto reader = whilelang::reader().file(input_path);

    try {
        auto result = reader.read();

        bool changes;
        do {
            changes = false;
            if (constant_analysis)
                result = result >> whilelang::optimization_analysis(changes);
        } while (changes);
        if (run)
            result = result >> whilelang::interpret();

        // If any result above was not ok it will carry through to here
        if (!result.ok) {
            trieste::logging::Error err;
            result.print_errors(err);
            trieste::logging::Debug() << result.ast;
            return 1;
        }
        trieste::logging::Debug() << "AST after all passes: " << std::endl
                                  << result.ast;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
