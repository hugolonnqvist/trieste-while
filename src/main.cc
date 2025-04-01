#include <trieste/driver.h>

#include "reader.cc"

int main(int argc, char** argv) {
    return trieste::Driver(whilelang::reader()).run(argc, argv);
}
