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

    std::set<Node> get_first_basic_children(Node n) {
        std::set<Node> children;

        while (n->type().in({Semi, Stmt, If})) {
            n = n->front();
        }

        if (n == While) {
            children.insert(n / BExpr);
        } else {
            children.insert(n);
        }

        return children;
    }
    Node get_last_basic_child(Node n) {
        while (n->type().in({Semi, Stmt, While})) {
            n = n->back();
        }
        return n;
    }

    std::set<Node> get_last_basic_children(const Node n) {
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

    void add_predecessor(
        std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor,
        Node node, Node prev) {
        auto res = predecessor->find(node);

        if (res != predecessor->end()) {
            res->second.insert(prev);
        } else {
            predecessor->insert({node, {prev}});
        }
    }

    void add_predecessor(
        std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor,
        Node node, std::set<Node> prevs) {
        auto res = predecessor->find(node);

        if (res != predecessor->end()) {
            for (auto prev : prevs) {
                res->second.insert(prev);
            }
        } else {
            predecessor->insert({node, prevs});
        }
    }

    void add_predecessor(
        std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor,
        std::set<Node> nodes, Node prev) {
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            auto res = predecessor->find(*it);

            if (res != predecessor->end()) {
                res->second.insert(prev);
            } else {
                predecessor->insert({*it, {prev}});
            }
        }
    }

    State get_incoming_state(Node node, std::set<Node> predecessor,
                             NodeMap<State> state_table, JoinTable join_table) {
        std::vector<State> states = std::vector<State>();

        for (auto it = predecessor.begin(); it != predecessor.end(); it++) {
            auto res = state_table.find(*it);

            if (res == state_table.end()) {
                throw std::runtime_error(
                    "Predecessor not found in state table");
            }
            State state = res->second;
            states.push_back(state);
        }

        State init_state = state_table.find(node)->second;
        return std::accumulate(states.begin(), states.end(), init_state,
                               [join_table](State s1, State s2) {
                                   return join(s1, s2, join_table);
                               });
    }

    State join(State s1, State s2, JoinTable join_table) {
        if (s1.size() != s2.size()) {
            throw std::runtime_error("State sizes do not match");
        }
        State result_state = State();
        for (auto it1 = s1.begin(), it2 = s2.begin();
             it1 != s1.end() && it2 != s2.end(); ++it1, ++it2) {
            if (it1->first != it2->first) {
                throw std::runtime_error("State keys do not match");
            }
            Token tok1 = it1->second;
            Token tok2 = it2->second;

            Token result = join_table[tok1][tok2];
            result_state.insert({it1->first, result});
        }
        return result_state;
    }

    int get_int_value(const Node& node) {
        std::string text(node->location().view());
        return std::stoi(text);
    }

    std::string get_identifier(const Node& node) {
        std::string text(node->location().view());
        return text;
    }

    void set_all_states_to_bottom(std::shared_ptr<Nodes> instructions,
                                  std::shared_ptr<std::set<std::string>> vars,
                                  NodeMap<State>& state_table) {
        for (size_t i = 1; i < instructions->size(); i++) {
            auto inst = instructions->at(i);

            State state = State();
            for (auto it = vars->begin(); it != vars->end(); it++) {
                state.insert({*it, TBottom});
            }
            state_table.insert({inst, state});
        }
    }

    bool states_equal(State s1, State s2) {
        if (s1.size() != s2.size()) {
            return false;
        }

        for (auto it1 = s1.begin(), it2 = s2.begin();
             it1 != s1.end() && it2 != s2.end(); ++it1, ++it2) {
            if (it1->first != it2->first || it1->second != it2->second) {
                return false;
            }
        }

        return true;
    }

    void log_instructions(Nodes instructions) {
        logging::Debug() << "Instructions: ";
        for (size_t i = 0; i < instructions.size(); i++) {
            logging::Debug() << i + 1 << "\n" << instructions[i] << std::endl;
        }
    }

    std::string lattice_to_str(Token val) {
        if (val == TTop) {
            return "T";
        } else if (val == TZero) {
            return "0";
        } else if (val == TBottom) {
            return "⊥";
        } else {
            return "N";
        }
    }

    void log_state_table(Nodes instructions, NodeMap<State> state_table) {
        int width = 8;
        int number_of_vars = state_table[instructions[0]].size();
        std::stringstream str;

        str << std::left << std::setw(width) << "";
        for (const auto& [key, _] : state_table[instructions[0]]) {
            str << std::setw(width) << key;
        }

        str << std::endl;
        str << std::string(width * (number_of_vars + 1), '-') << std::endl;

        // print values:
        for (size_t i = 0; i < instructions.size(); i++) {
            str << std::setw(width) << i + 1;
            for (const auto& [key, val] : state_table[instructions[i]]) {
                str << std::setw(width) << lattice_to_str(val);
            }
            str << '\n';
        }
        logging::Debug() << str.str();
    }

    void log_predecessors_and_successors(
        std::shared_ptr<Nodes> instructions,
        std::shared_ptr<NodeMap<NodeSet>> predecessors,
        std::shared_ptr<NodeMap<NodeSet>> successors) {
        for (size_t i = 0; i < instructions->size(); i++) {
            logging::Debug() << "Instructions: ";
            logging::Debug() << i + 1 << "\n"
                             << (*instructions)[i];

            logging::Debug() << "Predecessors: {" << std::endl;

            auto pred = predecessors->find((*instructions)[i]);

            if (pred == predecessors->end()) {
                logging::Debug() << "No predecessors" << std::endl;
            } else {
                for (auto it = pred->second.begin(); it != pred->second.end();
                     it++) {
                    logging::Debug() << *it << " " << std::endl;
                }
                logging::Debug() << "}" << std::endl;
            }

            logging::Debug() << "Sucessors: {" << std::endl;
            auto succ = successors->find((*instructions)[i]);
            if (succ == successors->end()) {
                logging::Debug() << "No successors" << std::endl;
            } else {
                for (auto it = succ->second.begin(); it != succ->second.end();
                     it++) {
                    logging::Debug() << *it << " " << std::endl;
                }
                logging::Debug() << "}" << std::endl;
            }
        }
    }
}
