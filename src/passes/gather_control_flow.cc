#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    PassDef gather_functions(std::shared_ptr<ControlFlow> cfg) {
        auto fun_defs = std::make_shared<NodeSet>();
        auto fun_calls = std::make_shared<NodeSet>();

        PassDef gather_functions = {
            "gather_functions",
            normalization_wf,
            dir::topdown | dir::once,
            {
                T(FunDef)[FunDef] >> [=](Match &_) -> Node {
                    fun_defs->insert(_(FunDef));
                    return NoChange;
                },
                T(FunCall)[FunCall] >> [=](Match &_) -> Node {
                    auto fun_id = _(FunCall) / FunId;

                    fun_calls->insert(_(FunCall));
                    return NoChange;
                },
            }};

        gather_functions.pre([=](Node) {
            if (cfg->is_dirty()) {
                cfg->clear();
            }
            return 0;
        });

        gather_functions.post([=](Node) {
            cfg->set_functions_calls(fun_defs, fun_calls);
            return 0;
        });

        return gather_functions;
    }

    PassDef gather_instructions(std::shared_ptr<ControlFlow> cfg) {
        PassDef gather_instructions = {
            "gather_instructions",
            normalization_wf,
            dir::topdown | dir::once,
            {
                T(FunDef)[FunDef] >> [=](Match &_) -> Node {
                    cfg->add_instruction(_(FunDef));
                    return NoChange;
                },

                T(Assign) << (T(Ident) * (T(AExpr) << T(FunCall)[FunCall])) >>
                    [=](Match &_) -> Node {
                    cfg->add_instruction(_(FunCall));

                    return NoChange;
                },

                T(Var, Assign, Skip, Output, Return)[Inst] >> [=](Match &_) -> Node {
                    Node inst = _(Inst);
                    cfg->add_instruction(inst);

                    // Gather variables
                    if (inst->type() == Assign) {
                        auto ident = inst / Ident;
                        cfg->add_var(ident);
                    }

                    return NoChange;
                },

                In(Atom, BAtom, Param) * T(Ident)[Ident] >>
                    [=](Match &_) -> Node {
                    cfg->add_var(_(Ident));
                    return NoChange;
                },

                In(While, If) * T(BAtom)[Inst] >> [=](Match &_) -> Node {
                    cfg->add_instruction(_(Inst));
                    return NoChange;
                },
            }};

        gather_instructions.post([cfg](Node) {
            if (cfg->get_instructions().empty()) {
                throw std::runtime_error("Unexpected, missing instructions");
            }

            return 0;
        });
        return gather_instructions;
    }

    PassDef gather_flow_graph(std::shared_ptr<ControlFlow> cfg) {
        PassDef gather_flow_graph = {
            "gather_flow_graph",
            normalization_wf,
            dir::topdown | dir::once,
            {
                // Control flow inside of While
                T(While)[While] >> [=](Match &_) -> Node {
                    auto stmt = _(While) / Stmt;
                    auto cond = _(While) / BAtom;
                    auto body = _(While) / Do;

                    cfg->add_edge(get_last_basic_children(stmt), cond);
                    cfg->add_edge(cond, get_first_basic_child(body));
                    cfg->add_edge(get_last_basic_children(body), get_first_basic_child(stmt));

                    return NoChange;
                },

                // Control flow inside of If
                T(If)[If] >> [=](Match &_) -> Node {
                    auto cond = _(If) / BAtom;
                    auto then_stmt = _(If) / Then;
                    auto else_stmt = _(If) / Else;

                    cfg->add_edge(cond, get_first_basic_child(then_stmt));
                    cfg->add_edge(cond, get_first_basic_child(else_stmt));

                    return NoChange;
                },

                // Handles exits of functions
                T(Return)[Return] >> [=](Match &_) -> Node {
                    auto curr = _(Return);

                    while ((curr != FunDef)) {
                        curr = curr->parent();
                    }

                    for (auto fun_call : cfg->get_fun_calls_from_def(curr)) {
                        auto assign = fun_call->parent()->parent();
                        if (assign != Assign) {
                            throw std::runtime_error(
                                "Invalid function call, expected assignment as "
                                "parent");
                        }
						cfg->add_edge(_(Return), assign);
                    }

                    return NoChange;
                },

                T(FunDef)[FunDef] >> [=](Match &_) -> Node {
                    auto fun_def = _(FunDef);
                    auto first_body = get_first_basic_child(fun_def / Body);

					cfg->add_edge(fun_def, first_body);

                    return NoChange;
                },

                T(FunCall)[FunCall] >> [=](Match &_) -> Node {
                    auto fun_call = _(FunCall);
                    auto fun_def = cfg->get_fun_def(fun_call);

					cfg->add_edge(fun_call, fun_def);

                    return NoChange;
                },

                // General case of a sequence of statements
                In(Block) * T(Stmt)[Prev] * T(Stmt)[Post] >>
                    [=](Match &_) -> Node {
                    auto last_prevs = get_last_basic_children(_(Prev));
                    auto first_post = get_first_basic_child(_(Post));

					cfg->add_edge(last_prevs, first_post);

                    if (_(Post) / Stmt == Assign) {
                        auto ass = _(Post) / Stmt;
                        if ((ass / Rhs) / Expr == FunCall) {
							cfg->add_edge(last_prevs, ass);
                        }
                    }
                    return NoChange;
                },
            }};
        gather_flow_graph.post([=](Node) {
            cfg->set_dirty_flag(false);
            return 0;
        });

        return gather_flow_graph;
    }
}
