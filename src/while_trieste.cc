#include "lang.hh"

#include <trieste/driver.h>

int main(int argc, char **argv) {
    auto vars_map = std::make_shared<std::map<std::string, std::string>>();
    return trieste::Driver(whilelang::reader(vars_map, false, false))
        .run(argc, argv);
}
