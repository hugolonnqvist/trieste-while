#include "control_flow.hh"

#include "utils.hh"

namespace whilelang {
    using namespace trieste;

    // Public
    ControlFlow::ControlFlow() {
        this->instructions = Nodes();
        this->vars = Vars();
        this->predecessor = NodeMap<NodeSet>();
        this->successor = NodeMap<NodeSet>();
    }

    void ControlFlow::add_var(Node ident) {
        vars.insert(get_identifier(ident));
    };

    void ControlFlow::add_predecessor(Node node, Node prev) {
        append_to_nodemap(predecessor, node, prev);
    }

    void ControlFlow::add_predecessor(Node node, NodeSet prevs) {
        append_to_nodemap(predecessor, node, prevs);
    }

    void ControlFlow::add_predecessor(NodeSet nodes, Node prev) {
        append_to_nodemap(predecessor, nodes, prev);
    }

    void ControlFlow::add_successor(Node node, Node succ) {
        append_to_nodemap(successor, node, succ);
    }

    void ControlFlow::add_successor(Node node, NodeSet succ) {
        append_to_nodemap(successor, node, succ);
    }

    void ControlFlow::add_successor(NodeSet nodes, Node succ) {
        append_to_nodemap(successor, nodes, succ);
    }

    void ControlFlow::log_predecessors_and_successors() {
        for (size_t i = 0; i < instructions.size(); i++) {
            auto inst = instructions[i];
            logging::Debug() << "Instruction: " << i + 1 << "\n" << inst;
            logging::Debug() << "Predecessors: {" << std::endl;

            auto pred = predecessor.find(inst);

            if (pred == predecessor.end()) {
                logging::Debug() << "No predecessors" << std::endl;
            } else {
                for (auto p : pred->second) {
                    logging::Debug() << p << " " << std::endl;
                }
                logging::Debug() << "}" << std::endl;
            }

            logging::Debug() << "Sucessors: {" << std::endl;

            auto succ = successor.find(inst);

            if (succ == successor.end()) {
                logging::Debug() << "No successors" << std::endl;
            } else {
                for (auto s : succ->second) {
                    logging::Debug() << s << " " << std::endl;
                }
                logging::Debug() << "}" << std::endl;
            }
        }
    }

    void ControlFlow::log_instructions() {
        logging::Debug() << "Instructions: ";
        for (size_t i = 0; i < instructions.size(); i++) {
            logging::Debug() << i + 1 << "\n" << instructions[i] << std::endl;
        }
    }

    // Private

    void ControlFlow::append_to_nodemap(NodeMap<NodeSet>& map, Node key,
                                        Node value) {
        auto res = map.find(key);

        if (res != map.end()) {
            // Append to nodemap entry if already exist
            res->second.insert(value);
        } else {
            // Otherwise create new entry
            map.insert({key, {value}});
        }
    }

    void ControlFlow::append_to_nodemap(NodeMap<NodeSet>& map, Node key,
                                        NodeSet values) {
        auto res = map.find(key);

        if (res != map.end()) {
            res->second.insert(values.begin(), values.end());
        } else {
            map.insert({key, values});
        }
    }

    void ControlFlow::append_to_nodemap(NodeMap<NodeSet>& map, NodeSet nodes,
                                        Node value) {
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            append_to_nodemap(map, *it, value);
        }
    }
}
