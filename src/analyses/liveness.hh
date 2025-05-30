#pragma once
#include "../utils.hh"
#include "dataflow_analysis.hh"

namespace whilelang {
    using State = Vars;
    using StateTable = DataFlowAnalysis<Vars, std::string>::StateTable;

    inline bool live_state_join(State &s1, const State &s2) {
        bool changed = s1 != s2;
        s1.insert(s2.begin(), s2.end());

        return changed;
    }

    inline Vars get_atom_defs(const Node &atom) {
        if (atom / Expr == Ident) {
            return {get_identifier(atom / Expr)};
        }
        return {};
    }

    inline Vars get_aexpr_op_defs(const Node &op) {
        auto lhs = get_atom_defs(op / Lhs);
        auto rhs = get_atom_defs(op / Rhs);
        live_state_join(lhs, rhs);

        return lhs;
    }

    inline Vars get_aexpr_defs(const Node &inst) {
        if (inst == Atom) {
            return get_atom_defs(inst);
        } else if (inst->type().in({Add, Sub, Mul})) {
            return get_aexpr_op_defs(inst);
        } else if (inst == FunCall) {
            auto args = inst / ArgList;
            auto defs = Vars();

            for (auto arg : *args) {
                auto arg_expr = (arg / Atom) / Expr;
                if (arg_expr == Ident) {
                    auto var = get_identifier(arg_expr);
                    defs.insert(var);
                }
            }
            return defs;
        } else {
            throw std::runtime_error(
                "Unexpected token, expected that parent would be of type "
                "aexpr");
        }
    }

    inline Vars live_flow(
        const Node &inst,
        StateTable &state_table,
        std::shared_ptr<ControlFlow>) {
        Vars new_defs = state_table[inst];
        Vars gen_defs = {};

        if (inst == Assign) {
            auto rhs = inst / Rhs;
            auto var = get_identifier(inst / Ident);

            gen_defs = get_aexpr_defs(rhs / Expr);

            new_defs.erase(var);
        } else if (inst->type().in({Output, Return})) {
            gen_defs = get_atom_defs(inst / Atom);
        } else if (inst == BExpr) {
            auto expr = inst / Expr;
            if (expr->type().in({LT, Equals})) {
                gen_defs = get_aexpr_op_defs(expr);
            }
        } else if (inst == Skip) {
            return new_defs;
        }
        new_defs.insert(gen_defs.begin(), gen_defs.end());
        return new_defs;
    };

    inline State live_create_state(Vars) {
        return {};
    }

    inline std::ostream &
    operator<<(std::ostream &os, const std::set<std::string> &state) {
        os << "{ ";
        for (const auto &var : state) {
            os << var << " ";
        }
        os << "}";
        return os;
    }

    inline void log_liveness(
        std::shared_ptr<ControlFlow> cfg,
        std::shared_ptr<DataFlowAnalysis<Vars, std::string>> analysis) {
        std::stringstream str;
        const auto instructions = cfg->get_instructions();
        for (size_t i = 0; i < instructions.size(); i++) {
            auto inst = instructions[i];
            str << "Instruction: " << i + 1
                << " has the follwing live variables: \n";
            for (auto var : analysis->get_state(inst)) {
                str << " " << var;
            }
            str << "\n";
        }
        logging::Debug() << str.str();
    }
}
