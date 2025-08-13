#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    PassDef inlining(
        std::shared_ptr<CallGraph> call_graph,
        std::shared_ptr<ControlFlow> cfg) {
        // Creates assignments for the parameters of the function call
        auto assign_params = [](Node &params, Node &args) -> Node {
            Node builder = Stmt;
            auto param_it = params->begin();
            auto arg_it = args->begin();

            while (param_it != params->end()) {
                builder
                    << (Stmt
                        << (Assign << (*param_it / Ident)->clone()
                                   << (AExpr << (*arg_it / Atom)->clone())));
                param_it++;
                arg_it++;
            }
            return builder;
        };

        auto create_return_ident = [=]() -> Node { return Ident ^ "__ret"; };

        return {
            "inlining",
            normalization_wf,
            dir::topdown,
            {
                T(Stmt)
                        << (T(Assign) << T(Ident)[Ident] *
                                (T(AExpr) << T(FunCall)[FunCall])) >>
                    [=](Match &_) -> Node {
                    auto fun_call = _(FunCall);
                    auto fun_id = get_identifier(fun_call / FunId);


                    if (call_graph->can_fun_be_inlined(fun_id)) {
                        cfg->set_dirty_flag(true);

                        auto fun_def = cfg->get_fun_def(fun_call);
                        auto builder = assign_params(
                            fun_def / ParamList, fun_call / ArgList);

                        auto fun_body = ((fun_def / Body) / Stmt)->clone();

                        // Make sure all children of body are inlined
                        for (auto child : *fun_body) {
                            builder << (Inlining << child);
                        }

                        auto return_ident = create_return_ident();

                        // Replace the previous fun call with assignment
                        // to return ident
                        builder
                            << (Stmt
                                << (Assign
                                    << _(Ident)
                                    << (AExpr << (Atom << return_ident))));

                        return Seq << *builder;
                    } else {
                        return NoChange;
                    }
                },

                T(Inlining) << T(Stmt)[Stmt] >> [](Match &_) -> Node {
                    return Stmt << (Inlining << *_(Stmt));
                },

                // Replace returns with assignment to return ident
                T(Inlining) << T(Return)[Return] >> [=](Match &_) -> Node {
                    auto ident = create_return_ident();
                    return Assign << ident << (AExpr << *_(Return));
                },

                T(Inlining) << T(Block)[Block] >> [=](Match &_) -> Node {
                    Node res = Block;
                    for (auto child : *_(Block)) {
                        res << (Inlining << child);
                    }
                    return res;
                },

                T(Inlining) << T(If)[If] >> [](Match &_) -> Node {
                    auto batom = _(If) / BAtom;
                    auto then_stmt = _(If) / Then;
                    auto else_stmt = _(If) / Else;

                    return If << batom << (Inlining << then_stmt)
                              << (Inlining << else_stmt);
                },

                T(Inlining) << T(While)[While] >> [](Match &_) -> Node {
                    auto cond_stmt = _(While) / Stmt;
                    auto batom = _(While) / BAtom;
                    auto do_stmt = _(While) / Do;
                    return While << (Inlining << cond_stmt) << batom << (Inlining << do_stmt);
                },

                T(Inlining) << T(Assign, Skip, Output, Var)[Stmt] >>
                    [](Match &_) -> Node { return _(Stmt); },

            }};
    }
}
