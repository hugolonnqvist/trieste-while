#include "internal.hh"

namespace whilelang {

using namespace trieste;

Reader reader()
  {
    return {
      "while",
      {
        // Parsing
        expressions(),
        statements(),

        // Checking
        check_refs(),

        // Static analysis
        init_flow_graph(),
        gather_instructions(),
      },
      whilelang::parser(),
    };
  }

}
