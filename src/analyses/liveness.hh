#pragma once
#include "../utils.hh"
#include "dataflow_analysis.hh"

namespace whilelang {
    using LiveState = Vars;

    Vars get_atom_defs(const Node &atom) {
        if (atom / Expr == Ident) {
            return {get_identifier(atom / Expr)};
        }
        return {};
    }

    Vars get_expr_op_defs(const Node &op) {
        auto lhs = get_atom_defs(op / Lhs);
        auto rhs = get_atom_defs(op / Rhs);
        lhs.insert(rhs.begin(), rhs.end());

        return lhs;
    }

    Vars get_expr_defs(const Node &inst) {
        if (inst == Atom || inst == BAtom) {
            return get_atom_defs(inst);
        } else if (inst->type().in({Add, Sub, Mul, And, Or, LT, Equals})) {
            return get_expr_op_defs(inst);
        } else if (inst == Not) {
            return get_atom_defs(inst / BAtom);
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
                "Unexpected token, expected that parent would be an expression"
            );
        }
    }

    struct LiveImpl {
		using StateTable = NodeMap<LiveState>;

        static LiveState create_state(Vars) {
            return {};
        }

        static bool state_join(LiveState &s1, const LiveState &s2) {
            bool changed = s1 != s2;
            s1.insert(s2.begin(), s2.end());

            return changed;
        }

        static Vars flow(
            const Node &inst,
            StateTable &state_table,
            std::shared_ptr<ControlFlow>) {
            Vars new_defs = state_table[inst];
            Vars gen_defs = {};

            if (inst == Assign) {
                auto rhs = inst / Rhs;
                auto var = get_identifier(inst / Ident);

                gen_defs = get_expr_defs(rhs / Expr);

                new_defs.erase(var);
            } else if (inst->type().in({Output, Return})) {
                gen_defs = get_atom_defs(inst / Atom);
            } else if (inst == BExpr) {
                auto expr = inst / Expr;
                if (expr->type().in({LT, Equals})) {
                    gen_defs = get_expr_op_defs(expr);
                }
            } else if (inst == BAtom) {
                gen_defs = get_atom_defs(inst);
            } else if (inst == Skip) {
                return new_defs;
            }
            new_defs.insert(gen_defs.begin(), gen_defs.end());
            return new_defs;
        };
    };

    std::ostream &operator<<(std::ostream &os, const LiveState &state) {
        os << "{ ";
        for (const auto &var : state) {
            os << var << " ";
        }
        os << "}";
        return os;
    }
}
