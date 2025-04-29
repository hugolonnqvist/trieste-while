#include "../control_flow.hh"
#include "../data_flow_analysis.hh"
#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    enum class ZeroAbstractType { Bottom, Zero, NonZero, Top };

    struct ZeroLatticeValue {
        ZeroAbstractType type;

        bool operator==(const ZeroLatticeValue &other) const {
            return type == other.type;
        }

        friend std::ostream &
        operator<<(std::ostream &os, const ZeroLatticeValue &value) {
            switch (value.type) {
                case ZeroAbstractType::Top:
                    os << "?";
                    break;
                case ZeroAbstractType::Zero:
                    os << "0";
                    break;
                case ZeroAbstractType::NonZero:
                    os << "N";
                    break;
                case ZeroAbstractType::Bottom:
                    os << "_";
                    break;
            }
            return os;
        }

        static ZeroLatticeValue top() {
            return {ZeroAbstractType::Top};
        }
        static ZeroLatticeValue zero() {
            return {ZeroAbstractType::Zero};
        }
        static ZeroLatticeValue non_zero() {
            return {ZeroAbstractType::NonZero};
        }
        static ZeroLatticeValue bottom() {
            return {ZeroAbstractType::Bottom};
        }
    };

    using State = typename DataFlowAnalysis<ZeroLatticeValue>::State;

    auto int_to_abstract_type = [](const Node node) {
        return get_int_value(node) == 0 ? ZeroAbstractType::Zero :
                                          ZeroAbstractType::NonZero;
    };

    auto handle_atom = [](const Node atom,
                          State &incoming_state) -> ZeroLatticeValue {
        if (atom == Int) {
            return get_int_value(atom) == 0 ? ZeroLatticeValue::zero() :
                                              ZeroLatticeValue::non_zero();
        } else if (atom == Ident) {
            std::string rhs_var = get_var(atom);
            return incoming_state[rhs_var];
        } else {
            return ZeroLatticeValue::top();
        }
    };

    ZeroLatticeValue zero_join(ZeroLatticeValue x, ZeroLatticeValue y) {
        auto top = ZeroLatticeValue::top();
        auto zero = ZeroLatticeValue::zero();
        auto non_zero = ZeroLatticeValue::non_zero();
        auto bottom = ZeroLatticeValue::bottom();

        if (x == bottom) {
            return y;
        }
        if (y == bottom) {
            return x;
        }

        if (x == top || y == top) {
            return top;
        }

        if (x == zero && y == zero) {
            return zero;
        }
        if (x == non_zero && y == non_zero) {
            return non_zero;
        }

        return top;
    }

    State zero_flow(
        Node inst,
        NodeMap<State> state_table,
        std::shared_ptr<ControlFlow> cfg) {
        auto incoming_state = state_table[inst];
        if (inst == Assign) {
            std::string var = get_var(inst / Ident);
            Node rhs = (inst / Rhs) / Expr;

            if (rhs == Atom) {
                auto atom = rhs / Expr;
                incoming_state[var] = handle_atom(atom, incoming_state);
            } else if (rhs == FunCall) {
                auto prevs = cfg->predecessors(inst);
                ZeroLatticeValue val = ZeroLatticeValue::bottom();

                for (auto return_node : prevs) {
                    val = zero_join(
                        val,
                        handle_atom(
                            (return_node / Atom) / Expr, incoming_state));
                }

                auto pre_fun_call_state = state_table[rhs];
                pre_fun_call_state[var] = val;
                return pre_fun_call_state;
            }
        } else if (inst == FunCall) {
            auto params = cfg->get_fun_def(inst) / ParamList;
            auto args = inst / ArgList;

            for (size_t i = 0; i < params->size(); i++) {
                auto param_id = params->at(i) / Ident;
                auto arg = args->at(i) / Atom;
                auto var = get_var(param_id);

                incoming_state[var] = handle_atom(arg / Expr, incoming_state);
            }
        }
        return incoming_state;
    }

    PassDef z_analysis(std::shared_ptr<ControlFlow> control_flow) {
        auto analysis = std::make_shared<DataFlowAnalysis<ZeroLatticeValue>>(
            zero_join, zero_flow);

        PassDef z_analysis = {
            "z_analysis", normalization_wf, dir::topdown | dir::once, {}};

        z_analysis.pre([=](Node) {
            // control_flow->log_instructions();
            // control_flow->log_variables();
            // control_flow->log_predecessors_and_successors();
            // logging::Debug()
            //     << "--------------------------------------------------- ";
            return 0;
        });

        z_analysis.post([=](Node) {
            auto first_state = ZeroLatticeValue::top();
            auto bottom = ZeroLatticeValue::bottom();

            analysis->forward_worklist_algoritm(
                control_flow, first_state, bottom);
            control_flow->log_instructions();
            analysis->log_state_table(control_flow->get_instructions());

            return 0;
        });

        return z_analysis;
    }
}
