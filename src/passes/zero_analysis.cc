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
                    os << "⊥";
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

    State zero_flow(Node inst, State incoming_state) {
        if (inst == Assign) {
            std::string var = get_identifier(inst / Ident);
            Node rhs = (inst / Rhs) / Expr;

            if (rhs == Atom) {
                auto atom = rhs / Expr;
                if (atom == Int) {
                    incoming_state[var].type = get_int_value(atom) == 0 ?
                        ZeroAbstractType::Zero :
                        ZeroAbstractType::NonZero;
                } else if (atom == Ident) {
                    std::string rhs_var = get_identifier(atom);
                    incoming_state[var] = incoming_state[rhs_var];
                } else {
                    incoming_state[var].type = ZeroAbstractType::Top;
                }
            }
        }
        return incoming_state;
    }

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

    PassDef z_analysis(std::shared_ptr<ControlFlow> control_flow) {
        auto analysis = std::make_shared<DataFlowAnalysis<ZeroLatticeValue>>(
            zero_join, zero_flow);

        PassDef z_analysis = {
            "z_analysis", normalization_wf, dir::topdown | dir::once, {}};

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
