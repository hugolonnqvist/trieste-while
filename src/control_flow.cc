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
        this->fun_call_to_def = NodeMap<Node>();
        this->fun_def_to_calls = NodeMap<NodeSet>();
        this->dirty_flag = false;
    }

    void ControlFlow::clear() {
        instructions.clear();
        vars.clear();
        predecessor.clear();
        successor.clear();
        fun_call_to_def.clear();
        fun_def_to_calls.clear();
    }

    void ControlFlow::add_var(Node ident) {
        vars.insert(get_identifier(ident));
    };

    void ControlFlow::add_edge(const Node &u, const Node &v) {
        append_to_nodemap(successor, u, v);
        append_to_nodemap(predecessor, v, u);
    }

    void ControlFlow::add_edge(const Node &u, const NodeSet &v) {
        append_to_nodemap(successor, u, v);
        append_to_nodemap(predecessor, v, u);
    }
    void ControlFlow::add_edge(const NodeSet &u, const Node &v) {
        append_to_nodemap(successor, u, v);
        append_to_nodemap(predecessor, v, u);
    }

    // Fill the map with function calls to their definitions
    void ControlFlow::set_functions_calls(
        std::shared_ptr<NodeSet> fun_defs, std::shared_ptr<NodeSet> fun_calls) {
        for (auto fun_call : *fun_calls) {
            auto call_id_str = get_identifier(fun_call / FunId);

            for (auto fun_def : *fun_defs) {
                auto fun_def_str = get_identifier(fun_def / FunId);
                if (call_id_str == fun_def_str) {
                    append_to_nodemap(fun_def_to_calls, fun_def, fun_call);
                    fun_call_to_def.insert({fun_call, fun_def});
                }
            }
        }

        for (auto fun_def : *fun_defs) {
            if (get_identifier(fun_def / FunId) == "main") {
                this->program_entry = fun_def;
                this->program_exit = get_last_basic_child(fun_def / Body);
                return;
            }
        }
        throw std::runtime_error(
            "No main function found. Please define a main function.");
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
    void ControlFlow::log_variables() {
        logging::Debug() << "Variables: ";
        for (auto var : vars) {
            logging::Debug() << var << ",";
        }
    }

    void ControlFlow::log_functions() {
        logging::Debug() << "Functions: ";
        for (auto [fun_def, calls] : fun_def_to_calls) {
            logging::Debug() << fun_def;
            for (auto call : calls) {
                logging::Debug() << call << "\n";
            }
        }
    }

    // Private

    void ControlFlow::append_to_nodemap(
        NodeMap<NodeSet> &map, const Node &key, const Node &value) {
        auto res = map.find(key);

        if (res != map.end()) {
            // Append to nodemap entry if already exist
            res->second.insert(value);
        } else {
            // Otherwise create new entry
            map.insert({key, {value}});
        }
    }

    void ControlFlow::append_to_nodemap(
        NodeMap<NodeSet> &map, const Node &key, const NodeSet &values) {
        auto res = map.find(key);

        if (res != map.end()) {
            res->second.insert(values.begin(), values.end());
        } else {
            map.insert({key, values});
        }
    }

    void ControlFlow::append_to_nodemap(
        NodeMap<NodeSet> &map, const NodeSet &nodes, const Node &value) {
        for (auto &node : nodes) {
            append_to_nodemap(map, node, value);
        }
    }
}
