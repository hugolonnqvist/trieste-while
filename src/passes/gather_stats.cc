#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {

    using namespace trieste;

    PassDef gather_stats() {
        auto vars = std::make_shared<std::set<std::string>>();
        auto instructions = std::make_shared<NodeSet>();

        PassDef gather_stats = {
            "gather_stats",
            normalization_wf,
            dir::topdown | dir::once,
            {

                T(FunDef)[FunDef] >> [=](Match &_) -> Node {
                    instructions->insert(_(FunDef));
                    return NoChange;
                },

                T(Assign) << (T(Ident) * (T(AExpr) << T(FunCall)[FunCall])) >>
                    [=](Match &_) -> Node {
                    instructions->insert(_(FunCall));

                    return NoChange;
                },

                T(Var, Assign, Skip, Output, Return)[Inst] >> [=](Match &_) -> Node {
                    Node inst = _(Inst);
                    instructions->insert(inst);

                    // Gather variables
                    if (inst->type() == Assign) {
                        auto ident = inst / Ident;
                        auto str = get_identifier(ident);
                        vars->insert(str);
                    }

                    return NoChange;
                },

                (T(Atom, BAtom, Param))[Expr] << T(Ident)[Ident] >>
                    [=](Match &_) -> Node {
                    auto str = get_identifier(_(Ident));
                    vars->insert(str);
                    return NoChange;
                },

                In(While, If) * T(BAtom)[Inst] >> [=](Match &_) -> Node {
                    instructions->insert(_(Inst));
                    return NoChange;
                },
            }};

        gather_stats.post([=](Node) {
            logging::Debug() << "INST POST NORM: " << instructions->size();
            logging::Debug() << "VARS POST NORM: " << vars->size();
            return 0;
        });
        return gather_stats;
    }
}
