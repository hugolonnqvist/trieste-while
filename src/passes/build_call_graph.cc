#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    PassDef build_call_graph(std::shared_ptr<CallGraph> call_graph) {
        PassDef pass = {
            "build_call_graph",
            normalization_wf,
            dir::topdown | dir::once,
            {
                T(FunCall)[FunCall] >> [=](Match &_) -> Node {
                    auto callee = (_(FunCall) / FunId) / Ident;
                    auto curr = _(FunCall);

                    while (curr != FunDef) {
                        curr = curr->parent();
                    }

                    auto caller = (curr / FunId) / Ident;

                    call_graph->add_vertex(caller);
                    call_graph->add_vertex(callee);
                    call_graph->add_edge(caller, callee);

                    return NoChange;
                },
            }};

        pass.post([=](Node) {
            call_graph->calculate_non_inline_funs();
            Vertices non_inline_funs = call_graph->get_non_inline_funs();

            std::stringstream str_builder;
            str_builder << "The functions not allowed to be inlined are:\n";

            for (auto &fun : non_inline_funs) {
                str_builder << fun << ", ";
            }

            logging::Debug() << str_builder.str();
            return 0;
        });

        return pass;
    }
}
