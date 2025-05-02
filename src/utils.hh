#pragma once
#include "lang.hh"

namespace whilelang {
    using namespace ::trieste;

    Node get_first_basic_child(Node n);

    Node get_last_basic_child(Node n);

    NodeSet get_last_basic_children(Node n);

    int get_int_value(const Node &node);

    int calc_arithmetic_op(Token op, int x, int y);

    std::string get_identifier(const Node &node);

    std::string get_var(const Node ident);
    // std::string get_var(const Node ident, const Node fun_def);

    Node create_const_node(int value);

	void log_var_map(std::shared_ptr<std::map<std::string, std::string>> vars_map);
}
