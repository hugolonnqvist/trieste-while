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
        }

        bool is_dirty() {
            return dirty;
        };

        void set_dirty(bool new_dirty) {
            dirty = new_dirty;
        };

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

        void add_var(Node ident);

        void add_predecessor(Node node, Node prev);
        void add_predecessor(Node node, NodeSet prev);
        void add_predecessor(NodeSet node, Node prev);

        void add_successor(Node node, Node prev);
        void add_successor(Node node, NodeSet prev);
        void add_successor(NodeSet node, Node prev);

        void log_predecessors_and_successors();
        void log_instructions();

      private:
        Nodes instructions;
        Vars vars;
        NodeMap<NodeSet> predecessor;
        NodeMap<NodeSet> successor;
        bool dirty;

        void append_to_nodemap(NodeMap<NodeSet> &map, Node key, Node value);
        void append_to_nodemap(NodeMap<NodeSet> &map, Node key, NodeSet values);
        void append_to_nodemap(NodeMap<NodeSet> &map, NodeSet nodes, Node prev);
    };
}
