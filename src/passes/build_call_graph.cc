#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    PassDef build_call_graph(std::shared_ptr<CallGraph> call_graph) {
        auto find_surronding_fun = [](Node fun_call) -> Node {
            auto curr = fun_call;

            while (curr != FunDef) {
                curr = curr->parent();
            }
            return curr / FunId;
        };

        PassDef pass = {
            "build_call_graph",
            normalization_wf,
            dir::topdown | dir::once,
            {
                T(FunCall)[FunCall] >> [=](Match &_) -> Node {
                    auto caller = _(FunCall) / FunId;
                    auto surronding_fun = find_surronding_fun(_(FunCall));

                    call_graph->add_vertex(caller);
                    call_graph->add_vertex(surronding_fun);
                    call_graph->add_edge(surronding_fun, caller);

                    return NoChange;
                },
            }};

        pass.post([=](Node) {
            call_graph->calculate_non_inline_funs();
            return 0;
        });

        return pass;
    }
}
