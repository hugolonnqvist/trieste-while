
#include "../internal.hh"
namespace whilelang {

    using namespace trieste;

    PassDef gather_stats(std::shared_ptr<Nodes> instructions) {
        PassDef gather_stats = {
            "gather_stats",
            normalization_wf,
            dir::topdown | dir::once,
            {
                T(FunDef)[FunDef] >> [=](Match &_) -> Node {
                    instructions->push_back(_(FunDef)->clone());
                    return NoChange;
                },

                T(Assign) << (T(Ident) * (T(AExpr) << T(FunCall)[FunCall])) >>
                    [=](Match &_) -> Node {
                    instructions->push_back(_(FunCall)->clone());
                    return NoChange;
                },

                T(Assign, Skip, Output, Return)[Inst] >> [=](Match &_) -> Node {
                    instructions->push_back(_(Inst)->clone());
                    return NoChange;
                },

                In(While, If) * T(BExpr)[Inst] >> [=](Match &_) -> Node {
                    instructions->push_back(_(Inst)->clone());
                    return NoChange;
                },
            }};

        return gather_stats;
    }

}
