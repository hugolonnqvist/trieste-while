#include "utils.hh"

#include "lang.hh"

namespace whilelang {
    using namespace trieste;

    void append_to_set(std::set<Node> &set, std::set<Node> new_set) {
        set.insert(new_set.begin(), new_set.end());
    }

    Node get_first_basic_child(Node n) {
        while (n->type().in({Block, Stmt, While, If, Assign})) {
            if (n == Assign) {
                if ((n / Rhs) / Expr == FunCall) {
                    n = (n / Rhs) / Expr;
                }
                break;
            }

            n = n->front();
        }
        return n;
    }

    Node get_last_basic_child(Node n) {
        while (n->type().in({Block, Stmt, While})) {
            n = n->back();
        }
        return n;
    }

    NodeSet get_last_basic_children(const Node n) {
        std::set<Node> children;
        Node curr = n;

        while (curr->type().in({Block, Stmt})) {
            curr = curr->back();
        }

        if (curr == While) {
            children.insert(curr / BExpr);
        } else if (curr == If) {
            auto then_last_nodes = get_last_basic_children(curr / Then);
            auto else_last_nodes = get_last_basic_children(curr / Else);

            append_to_set(children, then_last_nodes);
            append_to_set(children, else_last_nodes);
        } else {
            children.insert(curr);
        }

        return children;
    }

    int get_int_value(const Node &node) {
        std::string text(node->location().view());
        return std::stoi(text);
    }

    int calc_arithmetic_op(Token op, int x, int y) {
        if (op == Add) {
            return x + y;
        } else if (op == Sub) {
            return x - y;
        } else if (op == Mul) {
            return x * y;
        } else {
            throw std::runtime_error("Not a valid operator");
        }
    }

    std::string get_identifier(const Node &node) {
        return std::string(node->location().view());
    }

    std::string get_var(const Node ident) {
        auto curr = ident;
        while (curr != FunDef) {
            curr = curr->parent();
        }

        auto fun_id = (curr / FunId) / Ident;
        return get_identifier(ident) + get_identifier(fun_id);
    };

    Node create_const_node(int value) {
        return Int ^ std::to_string(value);
    };
}
