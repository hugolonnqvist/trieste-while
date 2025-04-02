#include "./data-flow-analysis.hh"

namespace whilelang {

    DataFlowAnalaysis::DataFlowAnalaysis(JoinTable table) {
        this->join_table = table;
        this->state_table = NodeMap<State>();
        this->vars = std::set<std::string>();
    }

    void DataFlowAnalaysis::set_state_in_table(Node node, State state) {
        state_table[node] = state;
    }

    void DataFlowAnalaysis::add_var(Node ident) {
        vars.insert(get_identifier(ident));
    };

    State DataFlowAnalaysis::join(const State s1, const State s2) {
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

    // State get_incoming_state(Node node, std::set<Node> predecessor,
    //                          NodeMap<State> state_table, JoinTable
    //                          join_table) {
    //     std::vector<State> states = std::vector<State>();
    //
    //     for (auto it = predecessor.begin(); it != predecessor.end(); it++) {
    //         auto res = state_table.find(*it);
    //
    //         if (res == state_table.end()) {
    //             throw std::runtime_error(
    //                 "Predecessor not found in state table");
    //         }
    //         State state = res->second;
    //         states.push_back(state);
    //     }
    //
    //     State init_state = state_table.find(node)->second;
    //     return std::accumulate(states.begin(), states.end(), init_state,
    //                            [join_table](State s1, State s2) {
    //                                return join(s1, s2, join_table);
    //                            });
    // }

    void DataFlowAnalaysis::set_all_states_to_bottom(Nodes instructions) {
        for (auto inst : instructions) {
            State state = State();
            for (auto it = vars.begin(); it != vars.end(); it++) {
                state.insert({*it, TBottom});
            }
            state_table.insert({inst, state});
        }
    }

    std::string lattice_to_str(Token val) {
        if (val == TTop) {
            return "?";
        } else if (val == TZero) {
            return "0";
        } else if (val == TBottom) {
            return "⊥";
        } else {
            return "N";
        }
    }

    void DataFlowAnalaysis::set_state(Node inst, Token new_symbol) {
        State new_state = State();
        for (auto var : vars) {
            new_state[var] = new_symbol;
        }

        state_table.insert_or_assign(inst, new_state);
    }

    void DataFlowAnalaysis::log_state_table(Nodes instructions) {
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
}
