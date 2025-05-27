#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Rewriter inlining_rewriter() {
        auto call_graph = std::make_shared<CallGraph>();
        auto cfg = std::make_shared<ControlFlow>();
        auto cfg_is_dirty = [=](Node) { return cfg->is_dirty(); };

        Rewriter rewriter = {
            "inlining_rewriter",
            {
                gather_functions(cfg),
                gather_instructions(cfg),
                gather_flow_graph(cfg),

                build_call_graph(call_graph),
                inlining(call_graph, cfg),
            },
            whilelang::normalization_wf,
        };

        return rewriter;
    }
}
