#pragma once
#include "control_flow.hh"
#include "lang.hh"

namespace whilelang {
    using namespace trieste;

    struct StateValue {
        Token type;
        int value;
    };

    using State = std::map<std::string, StateValue>;
    using JoinFn = StateValue(StateValue, StateValue);
    using FlowFn = State(Node, State);

    class DataFlowAnalysis {
       public:
        DataFlowAnalysis();

        void init(const Nodes instructions, const Vars vars);

        const NodeMap<State> get_state_table() { return state_table; };

        State get_state_in_table(Node inst) { return state_table[inst]; };

        StateValue get_state_value(Node inst, std::string var);

        StateValue get_state_value(Node inst, Node ident);

        void set_state_in_table(Node node, State state);

        State join(const State s1, const State s2, JoinFn join_fn);

        void set_state(Node inst, Token type, int value);

        void forward_worklist_algoritm(
            std::shared_ptr<ControlFlow> control_flow, FlowFn flow_fn,
            JoinFn join_fn);

        void backward_worklist_algoritm(
            std::shared_ptr<ControlFlow> control_flow, FlowFn flow_fn,
            JoinFn join_fn);

       private:
        NodeMap<State> state_table;

        void init_state_table(Nodes instructions, Vars vars);
    };
}
