#include "lang.hh"

#include <trieste/driver.h>

int main(int argc, char **argv) {
    return trieste::Driver(whilelang::reader()).run(argc, argv);
}
