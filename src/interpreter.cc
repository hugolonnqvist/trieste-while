#include "internal.hh"

namespace whilelang {

using namespace trieste;

Rewriter interpret()
  {
    return {
      "interpreter",
      {
        eval()
      },
      whilelang::statements_wf,
    };
  }
}
