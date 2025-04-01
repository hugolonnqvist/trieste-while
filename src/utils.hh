#pragma once
#include "lang.hh"

namespace whilelang {
    using namespace ::trieste;

    typedef std::map<std::string, Token> State;
    typedef std::map<Token, std::map<Token, Token>> JoinTable;

    Node get_first_basic_child(Node n);

    std::set<Node> get_first_basic_children(Node n);

    Node get_last_basic_child(Node n);

    std::set<Node> get_last_basic_children(Node n);

    void add_predecessor(
        std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor,
        Node node, Node prev);

    void add_predecessor(
        std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor,
        Node node, std::set<Node> prevs);

    void add_predecessor(
        std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor,
        std::set<Node> nodes, Node prev);

    State get_incoming_state(Node node, std::set<Node> predecessor,
                             NodeMap<State> state_table, JoinTable join_table);

    State join(State s1, State s2, JoinTable join_table);

    int get_int_value(const Node& node);

    std::string get_identifier(const Node& node);

    bool states_equal(State s1, State s2);

    void set_all_states_to_bottom(std::shared_ptr<Nodes> instructions,
                                  std::shared_ptr<std::set<std::string>> vars,
                                  NodeMap<State>& state_table);

    void log_instructions(Nodes instructions);
    void log_state_table(Nodes instructions, NodeMap<State> state_table);

    void log_predecessors_and_successors(
        std::shared_ptr<Nodes> instructions,
        std::shared_ptr<NodeMap<NodeSet>> predecessors,
        std::shared_ptr<NodeMap<NodeSet>> successors);
}
