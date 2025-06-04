#include "../analyses/constant_propagation.hh"
#include "../analyses/dataflow_analysis.hh"
#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    PassDef constant_folding(std::shared_ptr<ControlFlow> cfg) {
        auto analysis = std::make_shared<
            DataFlowAnalysis<CPState, CPLatticeValue, CPImpl>>();

        auto fetch_instruction = [=](const Node &n) -> Node {
            auto curr = n;

            while (!curr->type().in(
                {Assign, BExpr, FunCall, FunDef, Output, Return})) {
                curr = curr->parent();
            }
            return curr;
        };

        PassDef constant_folding = {
            "constant_folding",
            normalization_wf,
            dir::bottomup | dir::once,
            {
                In(Atom) * T(Ident)[Ident] >> [=](Match &_) -> Node {
                    auto inst = fetch_instruction(_(Ident));
                    auto var = get_identifier(_(Ident));
                    auto lattice_value = analysis->get_state(inst)[var];

                    if (lattice_value.type == CPAbstractType::Constant) {
                        cfg->set_dirty_flag(true);
                        return create_const_node(*lattice_value.value);
                    } else {
                        return NoChange;
                    }
                },
            }};

        constant_folding.pre([=](Node) {
            CPState first_state = cp_first_state(cfg);

            analysis->forward_worklist_algoritm(cfg, first_state);

            // cfg->log_instructions();
            // analysis->log_state_table(cfg);

            return 0;
        });

        return constant_folding;
    }
}
