#include "../internal.hh"
namespace whilelang {

    using namespace trieste;

    inline auto apply_arith_op = [](Node op, int x, int y) -> int {
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

    inline auto apply_bool_op = [](Node op, int x, int y) {
        if (op == LT) {
            return x < y;
        } else if (op == Equals) {
            return x == y;
        } else {
            throw std::runtime_error(
                "Error, expected a boolean operation, but was" +
                std::string(op->type().str()));
        }
    };

    PassDef simplify() {
        return {
            "simplify",
            statements_wf,
            dir::bottomup | dir::once,
            {
                T(AExpr) << T(Add, Sub, Mul)[Op] >> [](Match &_) -> Node {
                    Node op = _(Op);
                    auto curr = op->front();

                    // Make sure binary ops have exactly two operands
                    for (auto it = op->begin() + 1; it != op->end(); it++) {
                        if (curr / Expr == Int && *it / Expr == Int) {
                            int lhs = get_int_value(curr / Expr);
                            int rhs = get_int_value(*it / Expr);
                            curr = AExpr << create_const_node(
                                       apply_arith_op(op, lhs, rhs));
                        } else {
                            curr = AExpr << (op->type() << curr << *it);
                        }
                    }

                    return curr;
                },

                T(LT, Equals)[Op] >> [](Match &_) -> Node {
                    auto op = _(Op);
                    auto lhs = op / Lhs;
                    auto rhs = op / Rhs;

                    if (lhs / Expr == Int && rhs / Expr == Int) {
                        if (apply_bool_op(
                                op,
                                get_int_value(lhs / Expr),
                                get_int_value(rhs / Expr))) {
                            return True;
                        } else {
                            return False;
                        }
                    }
                    return NoChange;
                },
            }};
    }
}
