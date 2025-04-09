#include "utils.hh"

#include "lang.hh"

namespace whilelang {
    using namespace trieste;

    void append_to_set(std::set<Node>& set, std::set<Node> new_set) {
        set.insert(new_set.begin(), new_set.end());
    }

    Node get_first_basic_child(Node n) {
        while (n->type().in({Semi, Stmt, While, If})) {
            n = n->front();
        }
        return n;
    }

    Node get_last_basic_child(Node n) {
        while (n->type().in({Semi, Stmt, While})) {
            n = n->back();
        }
        return n;
    }

    NodeSet get_last_basic_children(const Node n) {
        std::set<Node> children;
        Node curr = n;

        while (curr->type().in({Semi, Stmt})) {
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

    int get_int_value(const Node& node) {
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

    std::string get_identifier(const Node& node) {
		return std::string(node->location().view());
    }

    std::string z_analysis_tok_to_str(StateValue value) {
        if (value.type == TTop) {
            return "?";
        } else if (value.type == TZero) {
            return "0";
        } else if (value.type == TBottom) {
            return "⊥";
        } else {
            return "N";
        }
    }

    void log_z_state_table(const Nodes instructions,
                           NodeMap<State> state_table) {
        int width = 8;
        int number_of_vars = state_table[instructions[0]].size();
        std::stringstream str;

        str << std::left << std::setw(width) << "";
        for (const auto& [key, _] : state_table[instructions[0]]) {
            str << std::setw(width) << key;
        }

        str << std::endl;
        str << std::string(width * (number_of_vars + 1), '-') << std::endl;

        for (size_t i = 0; i < instructions.size(); i++) {
            str << std::setw(width) << i + 1;
            for (const auto& [_, st] : state_table[instructions[i]]) {
                str << std::setw(width) << z_analysis_tok_to_str(st);
            }
            str << '\n';
        }
        logging::Debug() << str.str();
    }

    std::string cp_analysis_tok_to_str(StateValue val) {
        if (val.type == TTop) {
            return "?";
        } else if (val.type == TConstant) {
            return std::to_string(val.value);
        } else if (val.type == TBottom) {
            return "⊥";
        } else {
            throw std::runtime_error("Unexpected token type");
        }
    }

    void log_cp_state_table(const Nodes instructions,
                            NodeMap<State> state_table) {
        int width = 8;
        int number_of_vars = state_table[instructions[0]].size();
        std::stringstream str;

        str << std::left << std::setw(width) << "";
        for (const auto& [key, _] : state_table[instructions[0]]) {
            str << std::setw(width) << key;
        }

        str << std::endl;
        str << std::string(width * (number_of_vars + 1), '-') << std::endl;

        for (size_t i = 0; i < instructions.size(); i++) {
            str << std::setw(width) << i + 1;
            for (const auto& [_, st] : state_table[instructions[i]]) {
                str << std::setw(width) << cp_analysis_tok_to_str(st);
                // str << std::setw(width) << symbol << std::endl;
            }
            str << '\n';
        }
        logging::Debug() << str.str();
    }

    bool state_equals(State s1, State s2) {
        for (auto it1 = s1.begin(), it2 = s2.begin();
             it1 != s1.end() && it2 != s2.end(); ++it1, ++it2) {
            auto [key1, val1] = *it1;
            auto [key2, val2] = *it2;

            if (key1 != key2) {
                throw std::runtime_error("State keys do not match");
            }

            if ((val1.type != val2.type) || (val1.value != val2.value)) {
                return false;
            }
        }
        return true;
    }

    Node create_const(int value) { return Int ^ std::to_string(value); };
}
