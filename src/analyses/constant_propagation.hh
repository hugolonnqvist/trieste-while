#pragma once
#include "../utils.hh"
#include "dataflow_analysis.hh"

namespace whilelang {
    enum class CPAbstractType { Bottom, Constant, Top };

    struct CPLatticeValue {
        CPAbstractType type;
        std::optional<int> value;

        inline bool operator==(const CPLatticeValue &other) const {
            if (type == other.type) {
                if (type == CPAbstractType::Constant) {
                    return value == other.value;
                } else {
                    return true;
                }
            }
            return false;
        }

        CPLatticeValue join(const CPLatticeValue &other) const {
            auto constant = CPAbstractType::Constant;
            auto bottom = CPAbstractType::Bottom;

            if (this->type == bottom) {
                return other;
            } else if (other.type == bottom) {
                return *this;
            } else if (
                this->type == constant && other.type == constant &&
                this->value == other.value) {
                return *this;
            }

            return CPLatticeValue::top();
        }

        friend std::ostream &
        operator<<(std::ostream &os, const CPLatticeValue &lattice_value) {
            switch (lattice_value.type) {
                case CPAbstractType::Top:
                    os << "?";
                    break;
                case CPAbstractType::Constant:
                    if (auto str = lattice_value.value) {
                        os << *str;
                    } else {
                        throw std::runtime_error(
                            "Error, CPLatticeValue can not be of type Constant "
                            "and not have integer value");
                    }
                    break;
                case CPAbstractType::Bottom:
                    os << "B";
                    break;
            }
            return os;
        }

        static CPLatticeValue top() {
            return {CPAbstractType::Top, std::nullopt};
        }
        static CPLatticeValue bottom() {
            return {CPAbstractType::Bottom, std::nullopt};
        }
        static CPLatticeValue constant(int v) {
            return {CPAbstractType::Constant, v};
        }
    };

    using CPState = std::map<std::string, CPLatticeValue>;

    CPLatticeValue atom_flow_helper(Node inst, CPState incoming_state) {
        if (inst == Atom) {
            Node expr = inst / Expr;

            if (expr == Int) {
                return CPLatticeValue::constant(get_int_value(expr));
            } else if (expr == Ident) {
                std::string rhs_ident = get_identifier(expr);
                return incoming_state[rhs_ident];
            }
        }

        return CPLatticeValue::top();
    }

    int apply_arith_op(Node op, int x, int y) {
        if (op == Add) {
            return x + y;
        } else if (op == Sub) {
            return x - y;
        } else if (op == Mul) {
            return x * y;
        } else {
            throw std::runtime_error(
                "Error, expected an arithmetic operation, but was" +
                std::string(op->type().str()));
        }
    };

    CPState cp_first_state(std::shared_ptr<ControlFlow> cfg) {
        auto first_state = CPState();

        for (auto var : cfg->get_vars()) {
            first_state[var] = CPLatticeValue::top();
        }
        return first_state;
    }

    struct CPImpl {
		using StateTable = NodeMap<CPState>;

        static CPState create_state(const Vars &vars) {
            CPState state = CPState();

            for (auto var : vars) {
                state[var] = CPLatticeValue::bottom();
            }
            return state;
        }

        static bool state_join(CPState &x, const CPState &y) {
            bool changed = false;

            auto it1 = x.begin();
            auto it2 = y.begin();

            while (it1 != x.end() && it2 != y.end()) {
                auto join_res = it1->second.join(it2->second);

                if (join_res != it1->second) {
                    it1->second = join_res;
                    changed = true;
                }
                it1++;
                it2++;
            }

            // if (it1 != x.end() || it2 != y.end()) {
            //     throw std::runtime_error("States are not comparable");
            // }

            return changed;
        }

        static CPState flow(
            const Node &inst,
            StateTable &state_table,
            std::shared_ptr<ControlFlow> cfg) {
            auto incoming_state = state_table[inst];

            if (inst == Assign) {
                std::string var = get_identifier(inst / Ident);

                auto expr = (inst / Rhs) / Expr;
                if (expr == Atom) {
                    incoming_state[var] =
                        atom_flow_helper(expr, incoming_state);
                } else if (expr->type().in({Add, Sub, Mul})) {
                    Node lhs = expr / Lhs;
                    Node rhs = expr / Rhs;

                    auto lhs_value = atom_flow_helper(lhs, incoming_state);
                    auto rhs_value = atom_flow_helper(rhs, incoming_state);

                    if (lhs_value.type == CPAbstractType::Constant &&
                        rhs_value.type == CPAbstractType::Constant) {
                        auto op_result = apply_arith_op(
                            expr, *lhs_value.value, *rhs_value.value);
                        incoming_state[var] =
                            CPLatticeValue::constant(op_result);
                    } else {
                        incoming_state[var] = CPLatticeValue::top();
                    }
                } else {
                    // Is function call
                    auto prevs = cfg->predecessors(inst);
                    CPLatticeValue val = CPLatticeValue::bottom();

                    // Join result of all return statements
                    for (auto prev : prevs) {
                        if (prev == Return) {
                            val = val.join(atom_flow_helper(
                                prev / Atom, state_table[prev]));
                        }
                    }
                    auto pre_fun_call_state = state_table[expr];
                    pre_fun_call_state[var] = val;
                    return pre_fun_call_state;
                }
            } else if (inst == FunCall) {
                auto params = cfg->get_fun_def(inst) / ParamList;
                auto args = inst / ArgList;

                for (size_t i = 0; i < params->size(); i++) {
                    auto param_id = params->at(i) / Ident;

                    auto var_dec = get_identifier(param_id);
                    auto arg = args->at(i) / Atom;

                    incoming_state[var_dec] =
                        atom_flow_helper(arg, incoming_state);
                }
            } else if (
                inst == FunDef &&
                get_identifier((inst / FunId) / Ident) != "main") {
                auto params = inst / ParamList;

                auto param_vars = Vars();
                for (auto param : *params) {
                    param_vars.insert(get_identifier(param / Ident));
                }

                for (auto &[key, val] : incoming_state) {
                    if (!param_vars.contains(key)) {
                        incoming_state[key] = CPLatticeValue::bottom();
                    }
                }
            }
            return incoming_state;
        }
    };

    std::ostream &operator<<(std::ostream &os, const CPState &state) {
        for (const auto &[_, value] : state) {
            os << std::setw(PRINT_WIDTH) << value;
        }
        return os;
    }
}
