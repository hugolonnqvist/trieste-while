#include "../analyses/constant_propagation.hh"
#include "../analyses/dataflow_analysis.hh"
#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    PassDef constant_folding(std::shared_ptr<ControlFlow> cfg) {
        auto analysis =
            std::make_shared<DataFlowAnalysis<State, CPLatticeValue>>(
                cp_create_state, cp_state_join, cp_flow);

        auto ident_to_const = [=](int value) -> Node {
            cfg->set_dirty_flag(true);
            return Atom << create_const_node(value);
        };

        auto atom_is_const = [=](Node inst, Node atom) -> bool {
            if ((atom / Expr) == Ident) {
                auto lattice_value = analysis->get_lattice_value(
                    inst, get_identifier(atom / Expr));

                return lattice_value.type == CPAbstractType::Constant;
            }
            return false;
        };

        auto try_atom_to_const = [=](Node inst, Node atom) -> Node {
            if ((atom / Expr) == Ident) {
                auto lattice_value = analysis->get_lattice_value(
                    inst, get_identifier(atom / Expr));

                if (lattice_value.type == CPAbstractType::Constant) {
                    cfg->set_dirty_flag(true);
                    return ident_to_const(*lattice_value.value);
                }
            }
            return atom;
        };

        PassDef constant_folding = {
            "constant_folding",
            normalization_wf,
            dir::bottomup | dir::once,
            {
                T(FunCall)[FunCall]
                        << (T(FunId)[FunId] * T(ArgList)[ArgList]) >>
                    [=](Match &_) -> Node {
                    Node args = ArgList;
                    bool changed = false;

                    for (auto arg : *_(ArgList)) {
                        if ((arg / Atom) / Expr == Ident) {
                            auto var = get_identifier((arg / Atom) / Expr);

                            auto lattice_value =
                                analysis->get_lattice_value(_(FunCall), var);

                            if (lattice_value.type ==
                                CPAbstractType::Constant) {
                                changed = true;
                                cfg->set_dirty_flag(true);
                                args
                                    << (Arg << ident_to_const(
                                            *lattice_value.value));
                                continue;
                            }
                        }

                        args << arg->clone();
                    }
                    return changed ? FunCall << _(FunId) << args : NoChange;
                },

                T(Assign)[Assign] << (T(Ident)[Ident] * T(AExpr)[AExpr]) >>
                    [=](Match &_) -> Node {
                    auto inst = _(Assign);
                    auto ident = _(Ident);
                    auto aexpr = _(AExpr);

                    // Integers remain unchanged
                    if (aexpr / Expr == Atom && (aexpr / Expr) / Expr == Int) {
                        return NoChange;
                    }

                    auto ident_lattice_val = analysis->get_lattice_value(
                        inst, get_identifier(ident));

                    if (ident_lattice_val.type == CPAbstractType::Constant) {
                        return Assign
                            << ident
                            << (AExpr
                                << ident_to_const(*ident_lattice_val.value));
                    }
                    if ((aexpr / Expr)->type().in({Add, Sub, Mul})) {
                        auto op = aexpr / Expr;
                        if (!atom_is_const(inst, op / Lhs) &&
                            !atom_is_const(inst, op / Rhs)) {
                            return NoChange;
                        } else {
                            return Assign
                                << ident
                                << (AExpr
                                    << (op->type()
                                        << try_atom_to_const(inst, op / Lhs)
                                        << try_atom_to_const(inst, op / Rhs)));
                        }
                    }

                    return NoChange;
                },

                T(Output)[Output] << (T(Atom)[Atom] << T(Ident)) >>
                    [=](Match &_) -> Node {
                    auto inst = _(Output);
                    auto atom = _(Atom);

                    return atom_is_const(inst, atom) ?
                        Output << try_atom_to_const(inst, atom) :
                        NoChange;
                },

                T(Return)[Return] << (T(Atom)[Atom] << T(Ident)) >>
                    [=](Match &_) -> Node {
                    auto inst = _(Return);
                    auto atom = _(Atom);

                    return atom_is_const(inst, atom) ?
                        Return << try_atom_to_const(inst, _(Atom)) :
                        NoChange;
                },

                T(BExpr)[BExpr]
                        << (T(LT, Equals)[Op]
                            << (T(Atom)[Lhs] * T(Atom)[Rhs])) >>
                    [=](Match &_) -> Node {
                    auto inst = _(BExpr);

                    if (!atom_is_const(inst, _(Lhs)) &&
                        !atom_is_const(inst, _(Rhs))) {
                        return NoChange;
                    }

                    return BExpr
                        << (_(Op)->type() << try_atom_to_const(inst, _(Lhs))
                                          << try_atom_to_const(inst, _(Rhs)));
                },
            }};

        // clang-format on
        constant_folding.pre([=](Node) {
            auto first_state = State();

            for (auto var : cfg->get_vars()) {
                first_state[var] = CPLatticeValue::top();
            }

            analysis->forward_worklist_algoritm(cfg, first_state);

            cfg->log_instructions();
            analysis->log_state_table(cfg);

            return 0;
        });

        return constant_folding;
    }
}
