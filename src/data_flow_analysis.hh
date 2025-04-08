#pragma once
#include "control_flow.hh"
#include "lang.hh"

namespace whilelang {
    using namespace trieste;

    struct StateValue {
        Token type;
        int value;
    };

    using JoinFn = StateValue(StateValue st1, StateValue st2);
    using State = std::map<std::string, StateValue>;

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

        void log_state_table(Nodes instructions);

       private:
        NodeMap<State> state_table;

        void init_state_table(Nodes instructions, Vars vars);

        std::string z_analysis_tok_to_str(StateValue value);
    };
}
