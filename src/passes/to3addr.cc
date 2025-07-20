#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef to3addr() {
        PassDef to3addr = {
            "to3addr",
            three_addr_wf,
            dir::bottomup | dir::once,
            {
                In(FunDef) * (T(Stmt) << T(Block)[Body]) >>
                  [](Match &_) -> Node {
                    auto body = _(Body);
                    if (body->at(0) / Stmt == Label) return NoChange;
                    body->push_front(Stmt << (Label ^ body->fresh()));

                    return Stmt << body;
                  },

                T(Stmt) <<
                  (T(If) << (T(BAtom)[BAtom] * T(Stmt)[Then] * T(Stmt)[Else])) >>
                    [](Match &_) -> Node {
                        auto then_label = Label ^ _(BAtom)->fresh();
                        auto else_label = Label ^ _(BAtom)->fresh();
                        auto end_label = Label ^ _(BAtom)->fresh();
                        auto cond_ident = Ident ^ _(BAtom)->fresh();

                        auto cond_assign = Stmt << (Assign << cond_ident
                                                           << (BExpr << _(BAtom)));

                        auto cond = (Stmt << (Cond << cond_ident->clone()
                                                   << then_label->clone()
                                                   << else_label->clone()));
                        return Seq << cond_assign
                                   << cond
                                   << (Stmt << then_label)
                                   << *(_(Then) / Stmt)
                                   << (Stmt << (Jump << end_label->clone()))
                                   << (Stmt << else_label)
                                   << *(_(Else) / Stmt)
                                   << (Stmt << (Jump << end_label->clone()))
                                   << (Stmt << end_label);
                    },

                T(Stmt) <<
                  (T(While) << (T(Stmt)[Stmt] * T(BAtom)[BAtom] * T(Stmt)[Do])) >>
                      [](Match &_) -> Node {
                            auto cond_label = Label ^ _(BAtom)->fresh();
                            auto do_label = Label ^ _(BAtom)->fresh();
                            auto end_label = Label ^ _(BAtom)->fresh();
                            auto cond_ident = Ident ^ _(BAtom)->fresh();

                            auto cond_stmt = _(Stmt);
                            auto body = _(Do);

                            auto cond_assign = Stmt << (Assign << cond_ident
                                                               << (BExpr << _(BAtom)));

                            auto cond = (Stmt << (Cond << cond_ident->clone()
                                                       << do_label->clone()
                                                       << end_label->clone()));
                            return Seq << (Stmt << cond_label)
                                       << *(cond_stmt / Stmt)
                                       << cond_assign
                                       << cond
                                       << (Stmt << do_label)
                                       << *(body / Stmt)
                                       << (Stmt << (Jump << cond_label->clone()))
                                       << (Stmt << end_label);
                      },

                T(Stmt) <<
                  (T(Return) << (T(Atom) << T(Ident)[Ident])) >>
                      [](Match &_) -> Node {
                            return Stmt << (Return << _(Ident));
                      },

                T(Stmt) <<
                  (T(Return) << (T(Atom) << T(Int)[Int])) >>
                      [](Match &_) -> Node {
                            auto tmp = Ident ^ _(Int)->fresh();
                            auto assign = Stmt << (Assign << tmp
                                                          << (AExpr << (Atom << _(Int))));
                            return Seq << assign
                                       << (Stmt << (Return << tmp->clone()));
                      },

                In(Block) * T(Stmt) << T(Block)[Block] >>
                    [](Match &_) -> Node {
                        return Lift << Block << *_(Block);
                    },
            },
        };

        return to3addr;
    }
}
