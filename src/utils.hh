#pragma once
#include "lang.hh"
#include "data_flow_analysis.hh"

namespace whilelang {
    using namespace ::trieste;

    Node get_first_basic_child(Node n);

    Node get_last_basic_child(Node n);

    NodeSet get_last_basic_children(Node n);

    int get_int_value(const Node& node);

	int calc_arithmetic_op(Token op, int x, int y);

    std::string get_identifier(const Node& node);

    void log_cp_state_table(const Nodes instructions, NodeMap<State> state_table);

	bool state_equals(State s1, State s2);
}
