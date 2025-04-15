#include "../internal.hh"

namespace whilelang {
    using namespace trieste;
    using Bindings = std::shared_ptr<std::map<std::string, int>>;

    std::string get_lexeme(Node n) {
        return std::string(n->location().view());
    }

    int eval_atom(Node n, Bindings bindings) {
        if (n != Atom)
            throw std::runtime_error("Not an atom: " + n->str());

        auto expr = n / Expr;

        if (expr == Int)
            return std::stoi(get_lexeme(expr));

        if (expr == Ident) {
            auto var = get_lexeme(expr);

            if (bindings->find(var) != bindings->end())
                return (*bindings)[var];
            else
                throw std::runtime_error("Undefined variable: " + var);
        }

        if (expr == Input) {
            std::cout << "input: ";
            int value;
            std::cin >> value;
            return value;
        }

        throw std::runtime_error("Invalid atom: " + expr->str());
    }

    int eval_aexpr(Node n, Bindings bindings) {
        if (n != AExpr)
            throw std::runtime_error(
                "Not an arithmetic expression: " + n->str());

        auto expr = n / Expr;
        if (expr == Atom)
            return eval_atom(expr, bindings);

        if (!expr->type().in({Add, Sub, Mul}))
            throw std::runtime_error(
                "Invalid arithmetic expression: " + expr->str());

        auto lhs = eval_atom(expr / Lhs, bindings);
        auto rhs = eval_atom(expr / Rhs, bindings);

        return expr == Add ? lhs + rhs : expr == Sub ? lhs - rhs : lhs * rhs;
    }

    bool eval_bexpr(Node n, Bindings bindings) {
        if (n != BExpr)
            throw std::runtime_error("Not a boolean expression");

        auto expr = n / Expr;

        if (expr == True)
            return true;

        if (expr == False)
            return false;

        if (expr == Not)
            return !eval_bexpr(expr / BExpr, bindings);

        if (expr->type().in({Equals, LT})) {
            auto lhs = eval_atom(expr / Lhs, bindings);
            auto rhs = eval_atom(expr / Rhs, bindings);
            return expr == Equals ? lhs == rhs : lhs < rhs;
        }

        if (expr->type().in({And, Or})) {
            bool result = expr == And;
            for (auto &e : *expr) {
                auto b = eval_bexpr(e, bindings);
                result = expr == And ? result && b : result || b;
            }
            return result;
        }

        throw std::runtime_error("Invalid boolean expression");
    }

    PassDef eval() {
        auto bindings = std::make_shared<std::map<std::string, int>>();

        return {
            "eval",
            eval_wf,
            dir::topdown,
            {T(Program) << T(Stmt)[Stmt] >>
                 [](Match &_) -> Node { return Eval << _(Stmt); },

             In(Eval) * T(Stmt) << T(Block)[Block] >>
                 [](Match &_) -> Node { return Seq << *_[Block]; },

             In(Eval) * T(Stmt) << T(Skip) >>
                 [](Match &) -> Node { return {}; },

             In(Eval) * T(Stmt)
                     << (T(Assign) << (T(Ident)[Ident] * T(AExpr)[Rhs])) >>
                 [bindings](Match &_) -> Node {
                 auto var = get_lexeme(_(Ident));
                 auto rhs = _(Rhs);
                 (*bindings)[var] = eval_aexpr(rhs, bindings);
                 return {};
             },

             In(Eval) * T(Stmt) << (T(Output) << T(Atom)[Expr]) >>
                 [bindings](Match &_) -> Node {
                 auto expr = _(Expr);
                 int result = eval_atom(expr, bindings);
                 std::cout << result << std::endl;
                 return {};
             },

             In(Eval) * T(Stmt)
                     << (T(If)
                         << (T(BExpr)[BExpr] * T(Stmt)[Then] *
                             T(Stmt)[Else])) >>
                 [bindings](Match &_) -> Node {
                 auto cond = _(BExpr);
                 auto then = _(Then);
                 auto else_ = _(Else);

                 auto result = eval_bexpr(cond, bindings);
                 return result ? then : else_;
             },

             In(Eval) * T(Stmt)[While]
                     << (T(While) << (T(BExpr)[BExpr] * T(Stmt)[Do])) >>
                 [bindings](Match &_) -> Node {
                 auto while_ = _(While);
                 auto cond = _(BExpr);
                 auto body = _(Do);

                 auto result = eval_bexpr(cond, bindings);
                 if (result) {
                     return Seq << body->clone() << while_;
                 } else {
                     return {};
                 }
             },

             In(Eval) << Any[Stmt] >> [](Match &_) -> Node {
                 return Error << (ErrorAst << _(Stmt))
                              << (ErrorMsg ^ "Could not evaluate statement");
             },

             T(Eval) << End >> [](Match &) -> Node { return {}; }}};
    }

}
