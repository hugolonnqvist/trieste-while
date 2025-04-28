#pragma once
#include "lang.hh"

namespace whilelang {
    using namespace trieste;

    using Vars = std::set<std::string>;

    class ControlFlow {
      public:
        ControlFlow();

        void clear() {
            instructions.clear();
            vars.clear();
            predecessor.clear();
            successor.clear();
            fun_call_to_def.clear();
        }

        const NodeSet successors(Node node) {
            return successor[node];
        };

        const NodeSet predecessors(Node node) {
            return predecessor[node];
        };

        const Nodes get_instructions() {
            return instructions;
        };

        const Vars get_vars() {
            return vars;
        };

        void add_instruction(Node inst) {
            instructions.push_back(inst);
        };

        Node get_fun_def(Node fun_call) {
            return fun_call_to_def[fun_call];
        };

        NodeSet get_fun_calls_from_def(Node fun_def) {
            return fun_def_to_calls[fun_def];
        };

        Node get_program_entry() {
            return program_entry;
        };

        void set_functions_calls(
            std::shared_ptr<NodeSet> fun_defs,
            std::shared_ptr<NodeSet> fun_calls);

        void add_var(Node ident, std::string tag);

        void add_predecessor(Node node, Node prev);
        void add_predecessor(Node node, NodeSet prev);
        void add_predecessor(NodeSet node, Node prev);

        void add_successor(Node node, Node prev);
        void add_successor(Node node, NodeSet prev);
        void add_successor(NodeSet node, Node prev);

        void log_predecessors_and_successors();
        void log_instructions();
        void log_variables();
        void log_functions();

      private:
        Node program_entry;
        Nodes instructions;
        Vars vars;
        NodeMap<Node> fun_call_to_def; // Maps fun calls to their declarations
        NodeMap<NodeSet> fun_def_to_calls; // Maps fun defs to their call sites
        NodeMap<NodeSet> predecessor;
        NodeMap<NodeSet> successor;

        void append_to_nodemap(NodeMap<NodeSet> &map, Node key, Node value);
        void append_to_nodemap(NodeMap<NodeSet> &map, Node key, NodeSet values);
        void append_to_nodemap(NodeMap<NodeSet> &map, NodeSet nodes, Node prev);
    };
}
