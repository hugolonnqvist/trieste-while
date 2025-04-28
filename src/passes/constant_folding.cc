#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    enum class CPAbstractType { Bottom, Constant, Top };

    struct CPLatticeValue {
        CPAbstractType type;
        std::optional<int> value;

        bool operator==(const CPLatticeValue &other) const {
            if (type == other.type) {
                if (type == CPAbstractType::Constant) {
                    return value == other.value;
                } else {
                    return true;
                }
            }
            return false;
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
                    os << "⊥";
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

    using State = typename DataFlowAnalysis<CPLatticeValue>::State;

    CPLatticeValue atom_flow_helper(Node inst, State incoming_state) {
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
        ;
    }

    auto apply_op = [](Node op, int x, int y) {
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

    State cp_flow(Node inst, State incoming_state, std::shared_ptr<ControlFlow>) {
        if (inst == Assign) {
            std::string ident = get_identifier(inst / Ident);

            auto expr = (inst / Rhs) / Expr;
            if (expr == Atom) {
                incoming_state[ident] = atom_flow_helper(expr, incoming_state);
            } else if (expr->type().in({Add, Sub, Mul})) {
                Node lhs = expr / Lhs;
                Node rhs = expr / Rhs;

                auto lhs_value = atom_flow_helper(lhs, incoming_state);
                auto rhs_value = atom_flow_helper(rhs, incoming_state);

                if (lhs_value.type == CPAbstractType::Constant &&
                    rhs_value.type == CPAbstractType::Constant) {
                    auto op_result =
                        apply_op(expr, *lhs_value.value, *rhs_value.value);
                    incoming_state[ident] = CPLatticeValue::constant(op_result);
                } else {
                    incoming_state[ident] = CPLatticeValue::top();
                }
            } else {
				// Is function call
                incoming_state[ident] = CPLatticeValue::top();
			}
        }
        return incoming_state;
    }

    CPLatticeValue cp_join(CPLatticeValue x, CPLatticeValue y) {
        CPAbstractType top = CPAbstractType::Top;
        CPAbstractType constant = CPAbstractType::Constant;
        CPAbstractType bottom = CPAbstractType::Bottom;

        if (x.type == bottom) {
            return y;
        } else if (y.type == bottom) {
            return x;
        }

        if (x.type == top || y.type == top) {
            return CPLatticeValue::top();
        }

        if (x.type == constant && y.type == constant) {
            if (x.value == y.value) {
                return x;
            } else {
                return CPLatticeValue::top();
            }
        }

        return CPLatticeValue::top();
    }

    PassDef constant_folding(std::shared_ptr<ControlFlow> control_flow) {
        auto analysis = std::make_shared<DataFlowAnalysis<CPLatticeValue>>(
            cp_join, cp_flow);

        auto ident_to_const = [=](int value) -> Node {
            return Atom << create_const_node(value);
        };

        auto try_atom_to_const = [=](Node inst, Node atom) -> Node {
            if ((atom / Expr) == Ident) {
                auto lattice_value = analysis->get_lattice_value(
                    inst, get_identifier(atom / Expr));

                if (lattice_value.type == CPAbstractType::Constant) {
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
				// FIXME:: Functions

                T(Assign)[Assign]
                        << (T(Ident)[Ident] *
                            (T(AExpr) << T(Add, Sub, Mul)[Op])) >>
                    [=](Match &_) -> Node {
                    auto inst = _(Assign);
                    auto ident = _(Ident);

                    auto lattice_value = analysis->get_lattice_value(
                        inst, get_identifier(ident));

                    if (lattice_value.type == CPAbstractType::Constant) {
                        return Assign
                            << ident
                            << (AExpr << ident_to_const(*lattice_value.value));
                    } else {
                        auto op = _(Op);
                        return Assign
                            << ident
                            << (AExpr
                                << (op->type()
                                    << try_atom_to_const(inst, op / Lhs)
                                    << try_atom_to_const(inst, op / Rhs)));
                    }
                },

                T(Output)[Output] << (T(Atom)[Atom] << T(Ident)) >>
                    [=](Match &_) -> Node {
                    auto inst = _(Output);

                    return Output << try_atom_to_const(inst, _(Atom));
                },

                T(BExpr)[BExpr]
                        << (T(LT, Equals)[Op]
                            << (T(Atom)[Lhs] * T(Atom)[Rhs])) >>
                    [=](Match &_) -> Node {
                    auto inst = _(BExpr);

                    return BExpr
                        << (_(Op)->type() << try_atom_to_const(inst, _(Lhs))
                                          << try_atom_to_const(inst, _(Rhs)));
                },
            }};

        // clang-format on
        constant_folding.pre([=](Node) {
            auto first_state = CPLatticeValue::top();
            auto bottom = CPLatticeValue::bottom();
            control_flow->log_instructions();

            analysis->forward_worklist_algoritm(
                control_flow, first_state, bottom);

            control_flow->log_instructions();
            analysis->log_state_table(control_flow->get_instructions());

            return 0;
        });

        constant_folding.post([=](Node) {
            control_flow->clear();
            return 0;
        });

        return constant_folding;
    }
}
