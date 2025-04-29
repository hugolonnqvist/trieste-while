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

    using State = typename DataFlowAnalysis<CPLatticeValue>::State;

    CPLatticeValue atom_flow_helper(Node inst, State incoming_state) {
        if (inst == Atom) {
            Node expr = inst / Expr;

            if (expr == Int) {
                return CPLatticeValue::constant(get_int_value(expr));
            } else if (expr == Ident) {
                std::string rhs_ident = get_var(expr);
                return incoming_state[rhs_ident];
            }
        }

        return CPLatticeValue::top();
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
        if (x.type == constant && y.type == constant && x.value == y.value) {
            return x;
        }

        return CPLatticeValue::top();
    }

    State cp_flow(
        Node inst,
        NodeMap<State> state_table,
        std::shared_ptr<ControlFlow> cfg) {
        auto incoming_state = state_table[inst];

        if (inst == Assign) {
            std::string var = get_var(inst / Ident);

            auto expr = (inst / Rhs) / Expr;
            if (expr == Atom) {
                incoming_state[var] = atom_flow_helper(expr, incoming_state);
            } else if (expr->type().in({Add, Sub, Mul})) {
                Node lhs = expr / Lhs;
                Node rhs = expr / Rhs;

                auto lhs_value = atom_flow_helper(lhs, incoming_state);
                auto rhs_value = atom_flow_helper(rhs, incoming_state);

                if (lhs_value.type == CPAbstractType::Constant &&
                    rhs_value.type == CPAbstractType::Constant) {
                    auto op_result =
                        apply_op(expr, *lhs_value.value, *rhs_value.value);
                    incoming_state[var] = CPLatticeValue::constant(op_result);
                } else {
                    incoming_state[var] = CPLatticeValue::top();
                }
            } else {
                // // Is function call
                auto prevs = cfg->predecessors(inst);
                CPLatticeValue val = CPLatticeValue::bottom();

                for (auto prev : prevs) {
                    if (prev == Return) {
                        val = cp_join(
                            val,
                            atom_flow_helper(prev / Atom, state_table[prev]));
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

                auto var_dec = get_var(param_id);
                auto arg = args->at(i) / Atom;

                incoming_state[var_dec] = atom_flow_helper(arg, incoming_state);
            }
        } else if (
            inst == FunDef &&
            ((inst / FunId) / Ident)->location().view() != "main") {
            auto params = inst / ParamList;

            auto param_vars = Vars();
            for (auto param : *params) {
                param_vars.insert(get_var(param / Ident));
            }

            for (auto &[key, val] : incoming_state) {
                if (!param_vars.contains(key)) {
                    logging::Debug()
                        << "Var: " << key << "ain't for this function";
                    incoming_state[key] = CPLatticeValue::bottom();
                }
            }
        }
        return incoming_state;
    }

    PassDef constant_folding(std::shared_ptr<ControlFlow> cfg) {
        auto analysis = std::make_shared<DataFlowAnalysis<CPLatticeValue>>(
            cp_join, cp_flow);

        auto ident_to_const = [=](int value) -> Node {
            return Atom << create_const_node(value);
        };

        auto try_atom_to_const = [=](Node inst, Node atom) -> Node {
            if ((atom / Expr) == Ident) {
                auto lattice_value =
                    analysis->get_lattice_value(inst, get_var(atom / Expr));

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
                T(FunCall)[FunCall]
                        << (T(FunId)[FunId] * T(ArgList)[ArgList]) >>
                    [=](Match &_) -> Node {
                    Node args = ArgList;

                    for (auto arg : *_(ArgList)) {
                        if ((arg / Atom) / Expr == Ident) {
                            auto var = get_var((arg / Atom) / Expr);

                            auto lattice_value =
                                analysis->get_lattice_value(_(FunCall), var);

                            if (lattice_value.type ==
                                CPAbstractType::Constant) {
                                args
                                    << (Arg << ident_to_const(
                                            *lattice_value.value));
                                continue;
                            }
                        }

                        args << arg;
                    }
                    return FunCall << _(FunId) << args;
                },

                T(Assign)[Assign]
                        << (T(Ident)[Ident] *
                            (T(AExpr) << T(Add, Sub, Mul)[Op])) >>
                    [=](Match &_) -> Node {
                    auto inst = _(Assign);
                    auto ident = _(Ident);

                    auto lattice_value =
                        analysis->get_lattice_value(inst, get_var(ident));

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

                T(Return)[Return] << (T(Atom)[Atom] << T(Ident)) >>
                    [=](Match &_) -> Node {
                    auto inst = _(Return);

                    return Return << try_atom_to_const(inst, _(Atom));
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

            analysis->forward_worklist_algoritm(cfg, first_state, bottom);

            cfg->log_instructions();
            analysis->log_state_table(cfg->get_instructions());

            return 0;
        });

        constant_folding.post([=](Node) {
            cfg->clear();
            return 0;
        });

        return constant_folding;
    }
}
