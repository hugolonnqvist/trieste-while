#include <trieste/driver.h>

#include "lang.hh"

int main(int argc, char** argv) {
    return trieste::Driver(whilelang::reader()).run(argc, argv);
}
