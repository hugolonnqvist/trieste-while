#pragma once
#include "lang.hh"
#include "utils.hh"

namespace whilelang {
    using namespace trieste;

    typedef std::set<std::string> Vars;
    typedef std::map<std::string, Token> State;
    typedef std::map<Token, std::map<Token, Token>> JoinTable;

    class DataFlowAnalaysis {
       public:
        DataFlowAnalaysis(JoinTable join_table);

		State get_state_in_table(Node node) { return state_table[node]; };

		void set_state_in_table(Node node, State state);

        void add_var(Node ident);

        State join(const State s1, const State s2);

        void set_all_states_to_bottom(Nodes instructions);

        void set_state(Node inst, Token new_symbol);

        void log_state_table(Nodes instructions);

       private:
        JoinTable join_table;
        Vars vars;
        NodeMap<State> state_table;
    };
}
