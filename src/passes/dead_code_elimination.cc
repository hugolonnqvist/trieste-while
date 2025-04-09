#include "../data_flow_analysis.hh"
#include "../internal.hh"
#include "../set_lattice.hh"

namespace whilelang {
    using namespace trieste;

    Vars join(Vars s1, Vars s2) {
        auto union_set = Vars();
        union_set.insert(s1.begin(), s1.end());
        union_set.insert(s2.begin(), s2.end());

        return union_set;
    }

    Vars get_atom_defs(Node atom) {
        if (atom / Expr == Ident) {
            return {get_identifier(atom / Expr)};
        }
        return {};
    }

    Vars get_aexpr_op_defs(Node op) {
        auto lhs = get_atom_defs(op / Lhs);
        auto rhs = get_atom_defs(op / Rhs);

        return join(lhs, rhs);
    }

    Vars get_aexpr_defs(Node inst) {
        if (inst == Atom) {
            return get_atom_defs(inst);
        } else if (inst->type().in({Add, Sub, Mul})) {
            return get_aexpr_op_defs(inst);
        } else {
            throw std::runtime_error(
                "Unexpected token, expected that parent would be of type "
                "aexpr");
        }
    }

    Vars flow(Node inst, Vars state) {
        Vars new_defs = state;
        Vars gen_defs = {};

        if (inst == Assign) {
            auto ident = inst / Ident;
            auto aexpr = inst / Rhs;

            auto var = get_identifier(ident);

            gen_defs = get_aexpr_defs((inst / Rhs) / Expr);

            new_defs.erase(var);
        } else if (inst == Output) {
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

    PassDef dead_code_elimination(std::shared_ptr<ControlFlow> control_flow) {
        auto set_lattice = std::make_shared<SetLattice>();

        // clang-format off
		PassDef dead_code_elimination = {
            "dead_code_elimination",
            dead_code_elim_wf,
            dir::topdown,
            {
				T(Stmt) << (T(Assign)[Assign] << (T(Ident)[Ident] * T(AExpr)[AExpr])) >>
					[=](Match &_) -> Node
					{
						control_flow->get_instructions();
						auto id = get_identifier(_(Ident));
						auto assign = _(Assign);

						if (set_lattice->out_set[assign].contains(id)) {
							return NoChange;
						} else {
							return {};
						}
						return _(AExpr);
					},
				// TODO Add branch simplification/elimination

			}
		};
        // clang-format on

        dead_code_elimination.pre([=](Node) {
            const auto instructions = control_flow->get_instructions();
            const Vars vars = control_flow->get_vars();

			set_lattice->init(instructions);

            std::deque<Node> worklist{instructions.back()};

            while (!worklist.empty()) {
                Node inst = worklist.front();
                worklist.pop_front();

                Vars in_state = set_lattice->out_set[inst];
                Vars out_state = flow(inst, in_state);
                set_lattice->in_set[inst] = out_state;

                for (Node pred : control_flow->predecessors(inst)) {
                    Vars succ_state = set_lattice->out_set[pred];
                    Vars new_succ_state = join(out_state, succ_state);

                    if (new_succ_state != succ_state) {
                        set_lattice->out_set[pred] = new_succ_state;
                        worklist.push_back(pred);
                    }
                }
            }

            return 0;
        });

        return dead_code_elimination;
    }
}
