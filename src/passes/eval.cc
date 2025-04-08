#include "../internal.hh"

namespace whilelang
{
    using namespace trieste;
    using Bindings = std::shared_ptr<std::map<std::string, int>>;

    std::string get_lexeme(Node n)
    {
        return std::string(n->location().view());
    }

    int eval_aexpr(Node n, Bindings bindings)
    {
        if (n != AExpr)
            throw std::runtime_error("Not an arithmetic expression");

        auto expr = n->front();
        if (expr == Int) return std::stoi(get_lexeme(expr));
        else if (expr == Ident)
        {
            auto var = get_lexeme(expr);

            if (bindings->find(var) != bindings->end())
                return (*bindings)[var];
            else
                throw std::runtime_error("Undefined variable");
        }
        else if (expr == Add)
        {
            int sum = 0;
            for (auto &e : *expr)
            {
                sum += eval_aexpr(e, bindings);
            }
            return sum;
        }
        else if (expr == Sub)
        {
            auto it = expr->begin();
            int result = eval_aexpr(*it++, bindings);
            while (it != expr->end()) {
                result -= eval_aexpr(*it++, bindings);
            }
            return result;
        }
        else if (expr == Mul)
        {
            int prod = 1;
            for (auto &e : *expr)
            {
                prod *= eval_aexpr(e, bindings);
            }
            return prod;
        }
        else if (expr == Input)
        {
            std::cout << "input: ";
            int value;
            std::cin >> value;
            return value;
        }
        else
        {
            throw std::runtime_error("Invalid arithmetic expression");
        }
    }

    bool eval_bexpr(Node n, Bindings bindings)
    {
        if (n != BExpr)
            throw std::runtime_error("Not a boolean expression");

        auto expr = n->front();
        if (expr == True) return true;
        else if (expr == False) return false;
        else if (expr == Not) return !eval_bexpr(expr / BExpr, bindings);
        else if (expr == Equals || expr == LT)
        {
            auto lhs = eval_aexpr(expr->at(0), bindings);
            auto rhs = eval_aexpr(expr->at(1), bindings);
            return expr == Equals ? lhs == rhs
                                  : lhs < rhs;
        }
        else if (expr == And || expr == Or)
        {
            bool result = expr == And;
            for (auto &e : *expr)
            {
                auto b = eval_bexpr(e, bindings);
                result = expr == And ? result && b
                                     : result || b;
            }
            return result;
        }
        else {
            throw std::runtime_error("Invalid boolean expression");
        }
    }

    PassDef eval()
    {
        auto bindings = std::make_shared<std::map<std::string, int>>();

        return {
            "eval",
            eval_wf,
            dir::topdown,
            {
                T(Program) << T(Stmt)[Stmt] >>
                  [](Match &_) -> Node
                  {
                      return Eval << _(Stmt);
                  },

                In(Eval) * T(Stmt) <<
                  T(Semi)[Semi] >>
                    [](Match &_) -> Node
                    {
                        return Seq << *_[Semi];
                    },

                In(Eval) * T(Stmt) <<
                  T(Skip) >>
                    [](Match &) -> Node
                    {
                        return {};
                    },

                In(Eval) * T(Stmt) <<
                  (T(Assign) << (T(Ident)[Ident] * T(AExpr)[Rhs])) >>
                    [bindings](Match &_) -> Node
                    {
                        auto var = get_lexeme(_(Ident));
                        auto rhs = _(Rhs);
                        (*bindings)[var] = eval_aexpr(rhs, bindings);
                        return {};
                    },

                In(Eval) * T(Stmt) <<
                  (T(Output) << T(AExpr)[Expr]) >>
                    [bindings](Match &_) -> Node
                    {
                        auto expr = _(Expr);
                        int result = eval_aexpr(expr, bindings);
                        std::cout << result << std::endl;
                        return {};
                    },

                In(Eval) * T(Stmt) <<
                  (T(If) << (T(BExpr)[BExpr] * T(Stmt)[Then] * T(Stmt)[Else])) >>
                    [bindings](Match &_) -> Node
                    {
                        auto cond = _(BExpr);
                        auto then = _(Then);
                        auto else_ = _(Else);

                        auto result = eval_bexpr(cond, bindings);
                        return result ? then : else_;
                    },

                In(Eval) * T(Stmt)[While] <<
                  (T(While) << (T(BExpr)[BExpr] * T(Stmt)[Do])) >>
                    [bindings](Match &_) -> Node
                    {
                        auto while_ = _(While);
                        auto cond = _(BExpr);
                        auto body = _(Do);

                        auto result = eval_bexpr(cond, bindings);
                        if (result) {
                            return Seq << body->clone()
                                       << while_;
                        } else {
                            return {};
                        }
                    },

                In(Eval) << Any[Stmt] >>
                  [](Match &_) -> Node
                  {
                      return Error << (ErrorAst << _(Stmt))
                                   << (ErrorMsg ^ "Could not evaluate statement");
                  },

                T(Eval) << End >>
                  [](Match &) -> Node
                  {
                      return {};
                  }
            }};
    }

}
