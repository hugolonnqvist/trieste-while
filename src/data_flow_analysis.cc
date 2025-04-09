#include "data_flow_analysis.hh"

#include "utils.hh"

namespace whilelang {
    using namespace trieste;

    DataFlowAnalysis::DataFlowAnalysis() {
        this->state_table = NodeMap<State>();
    }

    void DataFlowAnalysis::init(const Nodes instructions, const Vars vars) {
        if (instructions.empty()) {
            throw std::runtime_error("No instructions exist for this program");
        }
        init_state_table(instructions, vars);

        // Initial state has all unknown, i.e. all Top
        set_state(instructions[0], TTop, 0);
    }

    void DataFlowAnalysis::init_state_table(const Nodes instructions,
                                            const Vars vars) {
        for (auto inst : instructions) {
            State state = State();

            for (auto var : vars) {
                state.insert({var, {TBottom, 0}});
            }
            state_table.insert({inst, state});
        }
    }

    void DataFlowAnalysis::set_state_in_table(Node node, State state) {
        state_table[node] = state;
    }

    StateValue DataFlowAnalysis::get_state_value(Node inst, std::string var) {
        return state_table[inst][var];
    };

    StateValue DataFlowAnalysis::get_state_value(Node inst, Node ident) {
        if (ident != Ident) {
            throw std::runtime_error(
                "Error, can not get value of state from non ident node.");
        }
        auto var = get_identifier(ident);
        return state_table[inst][var];
    };
    State DataFlowAnalysis::join(const State s1, const State s2,
                                 JoinFn join_fn) {
        if (s1.size() != s2.size()) {
            throw std::runtime_error("State sizes do not match");
        }

        State result_state = State();

        for (auto it1 = s1.begin(), it2 = s2.begin();
             it1 != s1.end() && it2 != s2.end(); ++it1, ++it2) {
            const auto& [key1, val1] = *it1;
            const auto& [key2, val2] = *it2;

            if (key1 != key2) {
                throw std::runtime_error("State keys do not match");
            }

            result_state[key1] = join_fn(val1, val2);
        }
        return result_state;
    }

    void DataFlowAnalysis::set_state(const Node inst, Token type, int value) {
        State new_state = State();

        for (auto& [_, old_st] : state_table[inst]) {
            old_st = {type, value};
        }
    }

    std::string DataFlowAnalysis::z_analysis_tok_to_str(StateValue value) {
        if (value.type == TTop) {
            return "?";
        } else if (value.type == TZero) {
            return "0";
        } else if (value.type == TBottom) {
            return "⊥";
        } else {
            return "N";
        }
    }

    void DataFlowAnalysis::log_state_table(Nodes instructions) {
        int width = 8;
        int number_of_vars = state_table[instructions[0]].size();
        std::stringstream str;

        str << std::left << std::setw(width) << "";
        for (const auto& [key, _] : state_table[instructions[0]]) {
            str << std::setw(width) << key;
        }

        str << std::endl;
        str << std::string(width * (number_of_vars + 1), '-') << std::endl;

        for (size_t i = 0; i < instructions.size(); i++) {
            str << std::setw(width) << i + 1;
            for (const auto& [_, st] : state_table[instructions[i]]) {
                str << std::setw(width) << z_analysis_tok_to_str(st);
            }
            str << '\n';
        }
        logging::Debug() << str.str();
    }

    void DataFlowAnalysis::forward_worklist_algoritm(
        std::shared_ptr<ControlFlow> control_flow, FlowFn flow_fn,
        JoinFn join_fn) {
        const auto instructions = control_flow->get_instructions();
        const Vars vars = control_flow->get_vars();
        this->init(instructions, vars);

        std::deque<Node> worklist{instructions[0]};

        while (!worklist.empty()) {
            Node inst = worklist.front();
            worklist.pop_front();

            State in_state = this->get_state_in_table(inst);
            State out_state = flow_fn(inst, in_state);

            this->set_state_in_table(inst, out_state);

            for (Node succ : control_flow->successors(inst)) {
                State succ_state = this->get_state_in_table(succ);
                State new_succ_state =
                    this->join(out_state, succ_state, join_fn);

                if (!state_equals(new_succ_state, succ_state)) {
                    this->set_state_in_table(succ, new_succ_state);
                    worklist.push_back(succ);
                }
            }
        }
    }
}
