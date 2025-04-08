#include <trieste/trieste.h>
#include <CLI/CLI.hpp>

#include "lang.hh"

int main(int argc, char const* argv[]) {
    CLI::App app;

    std::filesystem::path input_path;
    app.add_option("input", input_path, "Path to the input file ")->required();

    bool run;
	bool constant_analysis;
    app.add_flag("-r,--run", run, "Run the program (prompting inputs).");
    app.add_flag("-c,--constant-analysis", constant_analysis, "Compile and run constant analysis on the program.");

	
	trieste::logging::set_log_level_from_string("Debug");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    auto reader = whilelang::reader().file(input_path);

    try {
        auto result = reader.read();

		bool tmp = false;
		if (constant_analysis) result = result >> whilelang::constant_analysis(tmp);

        if (run) result = result >> whilelang::interpret();


        // If any result above was not ok it will carry through to here
        if (!result.ok) {
            trieste::logging::Error err;
            result.print_errors(err);
            return 1;
        }
		trieste::logging::Debug() << result.ast;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
