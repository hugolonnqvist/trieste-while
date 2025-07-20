#include "lang.hh"
#include "utils.hh"

#include <CLI/CLI.hpp>
#include <trieste/trieste.h>
#include <vbcc.h>

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
    bool run_gather_stats = false;
    bool run_mermaid = false;
    app.add_flag("-r,--run", run, "Run the program (prompting inputs).");
    app.add_flag(
        "-s,--static-analysis",
        run_static_analysis,
        "Compile and run static analysis on the program.");
    app.add_flag(
        "-z,--zero-analysis",
        run_zero_analysis,
        "Enable zero analysis in the static analysis. ");

    app.add_flag(
        "-p, --print-stats",
        run_gather_stats,
        "Runs the gather stats pass displaying total instructions and "
        "variables after normalization. Needed for the ./stats.sh script.");
    app.add_flag(
        "-m, --mermaid",
        run_mermaid,
        "Runs the mermaid pass which parses the final AST into mermaid "
        "format ");

    std::filesystem::path output_path = "";
    app.add_flag(
        "-o,--output",
        output_path,
        "Output file for the compiled program. If not specified, "
        "the output will be the input file name with .trieste extension.");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    auto vars_map = std::make_shared<std::map<std::string, std::string>>();
    auto reader =
        whilelang::reader(vars_map, run_gather_stats, run_mermaid).file(input_path);

    try {
        auto program_empty = [](trieste::Node ast) -> bool {
            return ast->front()->empty();
        };

        auto result = reader.read();

        if (run_static_analysis) {
            do {
                result = result >>
                    whilelang::optimization_analysis(run_zero_analysis);
            } while (result.ok && result.total_changes > 0 &&
                     !program_empty(result.ast));
        }

        trieste::Rewriter compiler = whilelang::compiler();
        result = result >> compiler;

        trieste::logging::Debug() << "AST after compilation: " << std::endl
                                  << result.ast;

        // If any result above was not ok it will carry through to here
        if (!result.ok) {
            trieste::logging::Error err;
            result.print_errors(err);
            trieste::logging::Debug() << result.ast;
            return 1;
        }
        whilelang::log_var_map(vars_map);

        if (output_path.empty())
          output_path = input_path.stem().replace_extension(".trieste");

        std::ofstream f(output_path, std::ios::binary | std::ios::out);
        if (f)
        {
          // Write the AST to the output file.
          f << "vbcc" << std::endl
            << "VIR" << std::endl
            << result.ast;
        }
        else
        {
          trieste::logging::Error() << "Could not open " << output_path << " for writing."
                                    << std::endl;
          return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << "Program failed with an exception: " << e.what() << std::endl;
    }

    return 0;
}
