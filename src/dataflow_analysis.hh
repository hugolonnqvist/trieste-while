#pragma once
#include "control_flow.hh"

#define PRINT_WIDTH 15

namespace whilelang {
    using namespace trieste;

    template<typename State, typename LatticeValue>
    class DataFlowAnalysis {
      public:
        using StateTable = std::unordered_map<Node, State>;
        using CreateStateFn = std::function<State(const Vars &)>;
        // Joins the two state and stores the result in the first one. Returns a
        // bool stating if the resulting state is changed
        using StateJoinFn = std::function<bool(State &, const State &)>;
        using FlowFn = std::function<State(
            const Node &, StateTable &, std::shared_ptr<ControlFlow>)>;

        DataFlowAnalysis(
            CreateStateFn create_state, StateJoinFn state_join, FlowFn flow);

        LatticeValue
        get_lattice_value(const Node &inst, const std::string &var);

        State built_in_join(const State x, const State y);
        bool built_in_join_mut(State &x, const State y);

        const State get_state(Node instruction) {
            return state_table[instruction];
        };

        void forward_worklist_algoritm(
            std::shared_ptr<ControlFlow> cfg, State first_state);

        void backward_worklist_algoritm(
            std::shared_ptr<ControlFlow> cfg, State first_state);

        void log_state_table(std::shared_ptr<ControlFlow> cfg);

      private:
        StateTable state_table;
        CreateStateFn create_state_fn;
        StateJoinFn state_join_fn;
        FlowFn flow_fn;

        bool state_equals(State x, State y);

        void init(
            const Nodes instructions,
            const Vars vars,
            const Node program_entry,
            State first_state);
    };

    template<typename State, typename LatticeValue>
    DataFlowAnalysis<State, LatticeValue>::DataFlowAnalysis(
        CreateStateFn create_state, StateJoinFn state_join, FlowFn flow) {
        this->state_table = StateTable();
        this->create_state_fn = create_state;
        this->state_join_fn = state_join;
        this->flow_fn = flow;
    }

    template<typename State, typename LatticeValue>
    void DataFlowAnalysis<State, LatticeValue>::init(
        const Nodes instructions,
        const Vars vars,
        const Node program_start,
        State first_state) {
        if (instructions.empty()) {
            throw std::runtime_error("No instructions exist for this program");
        }

        for (auto inst : instructions) {
            state_table.insert({inst, this->create_state_fn(vars)});
        }

        state_table[program_start] = first_state;
    }

    template<typename State, typename LatticeValue>
    LatticeValue DataFlowAnalysis<State, LatticeValue>::get_lattice_value(
        const Node &inst, const std::string &var) {
        return state_table[inst][var];
    }

    // template<typename State, typename LatticeValue>
    // State
    // DataFlowAnalysis<State, LatticeValue>::join(const State x, const State y)
    // {
    //     State result_state = x;
    //
    //     for (const auto &[key, val_y] : y) {
    //         auto res = result_state.find(key);
    //         if (res != result_state.end()) {
    //             auto val_x = res->second;
    //             result_state[key] = join_fn(val_x, val_y);
    //         } else {
    //             throw std::runtime_error("State do not have the same keys");
    //         }
    //     }
    //     return result_state;
    // }
    //
    // template<typename State, typename LatticeValue>
    // bool
    // DataFlowAnalysis<State, LatticeValue>::join_mut(State &x, const State y)
    // {
    //     bool changed = false;
    //     for (const auto &[key, val_y] : y) {
    //         auto res = x.find(key);
    //         if (res != x.end()) {
    //             auto join_res = join_fn(res->second, val_y);
    //             if (!(res->second == join_res)) {
    //                 x[key] = join_res;
    //                 changed = true;
    //             }
    //         } else {
    //             throw std::runtime_error("State do not have the same keys");
    //         }
    //     }
    //     return changed;
    // }

    // template<typename State, typename LatticeValue>
    // bool DataFlowAnalysis<State, LatticeValue>::state_equals(State x, State
    // y) {
    //     if (x.size() != y.size()) {
    //         throw std::runtime_error("State sizes do not match");
    //     }
    //
    //     for (auto it1 = x.begin(), it2 = y.begin();
    //          it1 != x.end() && it2 != y.end();
    //          ++it1, ++it2) {
    //         auto [key1, val1] = *it1;
    //         auto [key2, val2] = *it2;
    //
    //         if (key1 != key2) {
    //             throw std::runtime_error("State keys do not match");
    //         }
    //
    //         if (val1 != val2) {
    //             return false;
    //         }
    //     }
    //     return true;
    // }

    template<typename State, typename LatticeValue>
    void DataFlowAnalysis<State, LatticeValue>::forward_worklist_algoritm(
        std::shared_ptr<ControlFlow> cfg, State first_state) {
        const auto instructions = cfg->get_instructions();
        const Vars vars = cfg->get_vars();

        std::deque<Node> worklist{cfg->get_program_entry()};
        this->init(instructions, vars, cfg->get_program_entry(), first_state);

        while (!worklist.empty()) {
            Node inst = worklist.front();
            worklist.pop_front();

            State out_state = flow_fn(inst, state_table, cfg);
            state_table[inst] = out_state;

            for (Node succ : cfg->successors(inst)) {
                bool changed =
                    this->state_join_fn(state_table[succ], out_state);

                if (changed) {
                    worklist.push_back(succ);
                }
            }
        }
    }
    template<typename State, typename LatticeValue>
    void DataFlowAnalysis<State, LatticeValue>::backward_worklist_algoritm(
        std::shared_ptr<ControlFlow> cfg, State first_state) {
        const auto instructions = cfg->get_instructions();
        const Vars vars = cfg->get_vars();

        std::deque<Node> worklist{instructions.begin(), instructions.end()};
        this->init(instructions, vars, cfg->get_program_exit(), first_state);

        while (!worklist.empty()) {
            Node inst = worklist.front();
            worklist.pop_front();

            Vars in_state = flow_fn(inst, state_table, cfg);

            for (Node pred : cfg->predecessors(inst)) {
                Vars &succ_state = state_table[pred];
                bool changed = state_join_fn(succ_state, in_state);

                if (changed) {
                    worklist.push_back(pred);
                }
            }
        }
    }

    template<typename State, typename LatticeValue>
    void DataFlowAnalysis<State, LatticeValue>::log_state_table(
        std::shared_ptr<ControlFlow> cfg) {
        auto instructions = cfg->get_instructions();
        const int number_of_vars = cfg->get_vars().size();
        std::stringstream str_builder;

        str_builder << std::left << std::setw(PRINT_WIDTH) << "";
        for (auto var : cfg->get_vars()) {
            str_builder << std::setw(PRINT_WIDTH) << var;
        }

        str_builder << std::endl;
        str_builder << std::string(PRINT_WIDTH * (number_of_vars + 1), '-')
                    << std::endl;

        for (size_t i = 0; i < instructions.size(); i++) {
            str_builder << std::setw(PRINT_WIDTH) << i + 1;
            str_builder << state_table[instructions[i]];
            str_builder << '\n';
        }
        logging::Debug() << str_builder.str();
    }
}
