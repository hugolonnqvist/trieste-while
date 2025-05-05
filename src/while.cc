#include "lang.hh"
#include "utils.hh"

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
        "-s,--static-analysis",
        run_static_analysis,
        "Compile and run static analysis on the program.");
    app.add_flag(
        "-z,--zero-analysis",
        run_zero_analysis,
        "Enable zero analysis in the static analysis. ");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    auto vars_map = std::make_shared<std::map<std::string, std::string>>();
    auto reader = whilelang::reader(vars_map).file(input_path);

    try {
        auto start_time = std::chrono::steady_clock::now();
        auto result = reader.read();
        auto parse_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time);

        auto program_empty = [](trieste::Node ast) -> bool {
            return ast->front()->empty();
        };

        auto instuctions_pre = std::make_shared<trieste::Nodes>();
        result >> whilelang::statistics_rewriter(instuctions_pre);

        start_time = std::chrono::steady_clock::now();

        if (run_static_analysis) {
            do {
                result = result >>
                    whilelang::optimization_analysis(run_zero_analysis);
            } while (result.ok && result.total_changes > 0 &&
                     !program_empty(result.ast));
        }
        auto analysis_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time);

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

        whilelang::log_var_map(vars_map);
        auto instuctions_post = std::make_shared<trieste::Nodes>();
        result >> whilelang::statistics_rewriter(instuctions_post);

        trieste::logging::Debug()
            << "Parse time (init parse up and including normalization): "
            << parse_duration << "\n"
            << "Static analysis time (starting post normalization): "
            << analysis_duration << "\n"
            << "The number of instructions post normalization are: "
            << instuctions_pre->size() << "\n"
            << "The number of instructions remaining are: "
            << instuctions_post->size() << "\n";

    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
