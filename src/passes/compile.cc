#include "../internal.hh"
#include <vbcc.h>

namespace whilelang {
    using namespace trieste;

    PassDef compile() {
        PassDef compile = {
            "VIR",
            vbcc::wfIR,
            dir::topdown,
            {
                T(Program)[Program] >>
                  [](Match &_) -> Node {
                    Node res = Seq;
                    Node printval = vbcc::Symbol << (vbcc::SymbolId ^ "@printval")
                                                 << (vbcc::String ^ "printval")
                                                 << (vbcc::String ^ "")
                                                 << vbcc::None // Varargs
                                                 << (vbcc::FFIParams << vbcc::Dyn)
                                                 << vbcc::None; // Return type
                    res << (vbcc::Lib << (vbcc::String ^ "")
                                      << (vbcc::Symbols << printval));
                    Node input = vbcc::Symbol << (vbcc::SymbolId ^ "@input")
                                                 << (vbcc::String ^ "input")
                                                 << (vbcc::String ^ "")
                                                 << vbcc::None // Varargs
                                                 << (vbcc::FFIParams)
                                                 << vbcc::I32; // Return type
                    res << (vbcc::Lib << (vbcc::String ^ "libwhile_lib.dylib")
                                      << (vbcc::Symbols << input));
                    for (auto child : *_(Program)) {
                        res << (Compile << child);
                    }
                    return res;
                },

                // Functions and blocks
                T(Compile) <<
                  T(FunDef)[FunDef] >>
                    [](Match &_) -> Node {
                        auto fun = _(FunDef);
                        auto fun_id = std::string((fun / FunId)->location().view());
                        auto params = fun / ParamList;
                        auto blocks = fun / Blocks;
                        auto idents = fun / Idents;

                        Node res_id = vbcc::FunctionId ^ ("@" + fun_id);
                        Node res_params = vbcc::Params;

                        for (auto param : *params) {
                            auto param_id = param / Ident;
                            res_params << (vbcc::Param << (vbcc::LocalId ^ param_id)
                                                       << (vbcc::I32));
                        }

                        Node res_type = vbcc::I32;

                        Node res_vars = vbcc::Vars;
                        for (auto ident : *idents) {
                            res_vars << (vbcc::LocalId ^ ident);
                        }

                        Node res_body = Compile << blocks;
                        return vbcc::Func << res_id << res_params << res_type << res_vars
                                          << res_body;
                    },

                T(Compile) <<
                  T(Blocks)[Blocks] >>
                    [](Match &_) -> Node {
                        Node res = vbcc::Labels;
                        for (auto child : *_(Blocks)) {
                            res << (Compile << child);
                        }
                        return res;
                    },

                T(Compile) <<
                  T(Block)[Block] >>
                    [](Match &_) -> Node {
                        auto label = _(Block) / Label;
                        auto body = _(Block) / Body;
                        auto terminator = _(Block) / Jump;

                        auto label_id = vbcc::LabelId ^ label;

                        Node res_body = vbcc::Body;
                        for (auto child : *body) {
                            res_body << (Compile << child);
                        }

                        return vbcc::Label << label_id
                                           << res_body
                                           << (Compile << terminator);
                    },

                // Terminators
                T(Compile) <<
                    T(Jump)[Jump] >>
                    [](Match &_) -> Node {
                        auto label = _(Jump) / Label;
                        auto label_id = vbcc::LabelId ^ label;
                        return vbcc::Jump << label_id;
                    },

                T(Compile) <<
                    T(Cond)[Cond] >>
                    [](Match &_) -> Node {
                        auto ident = _(Cond) / Ident;
                        auto then_label = _(Cond) / Then;
                        auto else_label = _(Cond) / Else;

                        return vbcc::Cond << (vbcc::LocalId ^ ident)
                                          << (vbcc::LabelId ^ then_label)
                                          << (vbcc::LabelId ^ else_label);
                    },

                T(Compile) <<
                    T(Return)[Return] >>
                    [](Match &_) -> Node {
                        auto ident = _(Return) / Ident;
                        return vbcc::Return << (vbcc::LocalId ^ ident);
                    },

                // Atoms
                T(Compile) <<
                    (T(Atom) << T(Int)[Int]) >>
                    [](Match &_) -> Node {
                        auto int_val = _(Int);
                        auto tmp = _(Int)->fresh();
                        auto _const = vbcc::Const << (vbcc::LocalId ^ tmp)
                                                  << vbcc::I32
                                                  << (vbcc::Int ^ int_val);
                        return Seq << (Lift << vbcc::Body << _const)
                                   << (vbcc::LocalId ^ tmp);
                    },

                T(Compile) <<
                    (T(Atom, BAtom) << T(Ident)[Ident]) >>
                    [](Match &_) -> Node {
                        return vbcc::LocalId ^ _(Ident);
                    },

                T(Compile) <<
                    (T(BAtom) << T(True, False)[Expr]) >>
                    [](Match &_) -> Node {
                        auto bool_val = _(Expr);
                        auto tmp = bool_val->fresh();
                        auto _const = vbcc::Const << (vbcc::LocalId ^ tmp)
                                                  << vbcc::Bool
                                                  << (bool_val == True ? vbcc::True : vbcc::False);
                        return Seq << (Lift << vbcc::Body << _const)
                                   << (vbcc::LocalId ^ tmp);
                    },

                T(Compile) <<
                    (T(Atom) << T(Input)) >>
                    [](Match &_) -> Node {
                        auto tmp = _.fresh();
                        auto name = vbcc::SymbolId ^ "@input";
                        Node args = vbcc::Args;
                        return Seq << (Lift << vbcc::Body
                                            << (vbcc::FFI << (vbcc::LocalId ^ tmp)
                                                          << name
                                                          << args))
                                   << (vbcc::LocalId ^ tmp);
                    },

                // Statements
                T(Compile) <<
                    (T(Stmt) << T(Skip)[Skip]) >>
                    [](Match &_) -> Node {
                        return {};
                    },

                T(Compile) << (T(Stmt) <<
                  (T(Assign) << (T(Ident)[Ident] *
                                (T(AExpr) << (T(Atom) << T(Int)[Int]))))) >>
                    [](Match &_) -> Node {
                        return vbcc::Const << (vbcc::LocalId ^ _(Ident))
                                           << vbcc::I32
                                           << (vbcc::Int ^ _(Int));
                    },

                T(Compile) << (T(Stmt) <<
                  (T(Assign) << (T(Ident)[Ident] *
                                (T(AExpr) << (T(Atom) << T(Ident)[Expr]))))) >>
                    [](Match &_) -> Node {
                        return vbcc::Copy << (vbcc::LocalId ^ _(Ident))
                                          << (vbcc::LocalId ^ _(Expr));
                    },

                T(Compile) << (T(Stmt) <<
                  (T(Assign) << (T(Ident)[Ident] *
                                (T(AExpr) << (T(Atom) << T(Input)))))) >>
                    [](Match &_) -> Node {
                        return vbcc::FFI << (vbcc::LocalId ^ _(Ident))
                                         << (vbcc::SymbolId ^ "@input")
                                         << vbcc::Args;
                    },

                T(Compile) << (T(Stmt) <<
                  (T(Assign) << (T(Ident)[Ident] *
                                (T(BExpr) << (T(BAtom) << T(Ident)[Expr]))))) >>
                    [](Match &_) -> Node {
                        return vbcc::Copy << (vbcc::LocalId ^ _(Ident))
                                          << (vbcc::LocalId ^ _(Expr));
                    },

                T(Compile) << (T(Stmt) <<
                  (T(Assign) << (T(Ident)[Ident] *
                                (T(BExpr) << (T(BAtom) << T(True, False)[Expr]))))) >>
                    [](Match &_) -> Node {
                        auto bool_val = _(Expr);
                        return vbcc::Const << (vbcc::LocalId ^ _(Ident))
                                           << vbcc::Bool
                                           << (vbcc::Int ^ (bool_val == True ? "1" : "0"));
                    },


                T(Compile) << (T(Stmt) <<
                  (T(Assign) << (T(Ident)[Ident] *
                                (T(AExpr) << T(FunCall)[FunCall])))) >>
                    [](Match &_) -> Node {
                        auto fun_call = _(FunCall);
                        auto fun_id = std::string((fun_call / FunId)->location().view());
                        auto args = fun_call / ArgList;

                        Node tmp = vbcc::LocalId ^ "$_";
                        Node name = vbcc::FunctionId ^ ("@" + fun_id);
                        Node args_node = vbcc::Args;

                        for (auto arg : *args) {
                            args_node << (vbcc::Arg << vbcc::ArgCopy << (Compile << (arg / Atom)));
                        }

                        auto dst = vbcc::LocalId ^ _(Ident);

                        return vbcc::Call << dst
                                          << name
                                          << args_node;
                    },

                T(Compile) << (T(Stmt) <<
                  (T(Assign) << (T(Ident)[Ident] * (T(BExpr) << T(Not)[Expr])))) >>
                    [](Match &_) -> Node {
                        auto ident = _(Ident);
                        auto batom = _(Expr) / BAtom;
                        auto dst = vbcc::LocalId ^ ident;
                        return vbcc::Not << dst
                                         << (Compile << batom);
                    },

                T(Compile) << (T(Stmt) <<
                  (T(Assign) << (T(Ident)[Ident] * T(AExpr, BExpr)[Expr]))) >>
                    [](Match &_) -> Node {
                        auto ident = _(Ident);
                        auto expr = _(Expr)->front();

                        Node op = expr == Add ? vbcc::Add :
                                  expr == Sub ? vbcc::Sub :
                                  expr == Mul ? vbcc::Mul :
                                  expr == LT ? vbcc::Lt :
                                  expr == Equals ? vbcc::Eq :
                                  expr == And ? vbcc::And :
                                  expr == Or ? vbcc::Or : throw std::runtime_error(std::string("Invalid operator: ") + expr->type().str());

                        auto dst = vbcc::LocalId ^ ident;
                        auto lhs = expr / Lhs;
                        auto rhs = expr / Rhs;
                        return op << dst << (Compile << lhs)
                                         << (Compile << rhs);
                    },

                T(Compile) << (T(Stmt) <<
                    (T(Output) << (T(Atom)[Atom]))) >>
                    [](Match &_) -> Node {
                        auto atom = _(Atom);
                        auto tmp = vbcc::LocalId ^ "$_";
                        auto name = vbcc::SymbolId ^ "@printval";
                        auto args = vbcc::Args << (vbcc::Arg << vbcc::ArgCopy << (Compile << atom));
                        return vbcc::FFI << tmp
                                         << name
                                         << args;
                    },

                T(Compile) << Any[Expr] >>
                    [](Match &_) -> Node {
                        return Error << (ErrorAst << _(Expr))
                                     << (ErrorMsg ^ "Cannot compile term:" + _(Expr)->str());
                    },
            },
        };

        return compile;
    }
}
