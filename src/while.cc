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
    bool run_static_analysis = false;
    bool run_zero_analysis = false;
    app.add_flag("-r,--run", run, "Run the program (prompting inputs).");
    app.add_flag(
        "-z,--zero-analysis",
        run_zero_analysis,
        "Enable zero analysis in the static analysis. ");
    app.add_flag(
        "-s,--static-analysis",
        run_static_analysis,
        "Compile and run static analysis on the program.");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }


    try {
		auto reader = whilelang::reader().file(input_path);
        auto result = reader.read();
		auto program_empty = [] (trieste::Node ast) -> bool {
			return ast->front()->empty();
		};

        if (run_static_analysis) {
            do {
                result = result >>
                    whilelang::optimization_analysis(run_zero_analysis);
            } while (result.ok && result.total_changes > 0 && !program_empty(result.ast));
        }

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
