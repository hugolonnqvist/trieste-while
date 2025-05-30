#pragma once
#include "../control_flow.hh"
#include "../internal.hh"

#define PRINT_WIDTH 15

namespace whilelang {
    using namespace trieste;

    template<typename State, typename LatticeValue>
    class DataFlowAnalysis {
      public:
        using StateTable = NodeMap<State>;
        using CreateStateFn = std::function<State(const Vars &)>;

        // Joins the two state and stores the result in the first one. Returns a
        // bool stating if the resulting state is changed
        using StateJoinFn = std::function<bool(State &, const State &)>;
        using FlowFn = std::function<State(
            const Node &, StateTable &, std::shared_ptr<ControlFlow>)>;

        DataFlowAnalysis(
            CreateStateFn create_state, StateJoinFn state_join, FlowFn flow);

        LatticeValue
        get_lattice_value(const Node &inst, const std::string &var) {
            return state_table[inst][var];
        }

        const State get_state(const Node &instruction) {
            return state_table[instruction];
        };

        void forward_worklist_algoritm(
            std::shared_ptr<ControlFlow> cfg, State first_state);

        void backward_worklist_algoritm(
            std::shared_ptr<ControlFlow> cfg, State first_state);

        void log_state_table(std::shared_ptr<ControlFlow> cfg);

      private:
        StateTable state_table;
        CreateStateFn create_state;
        StateJoinFn state_join;
        FlowFn flow;

        void init_state_table(
            const Nodes &instructions,
            const Vars &vars,
            const Node &program_entry,
            State first_state);
    };

    template<typename State, typename LatticeValue>
    DataFlowAnalysis<State, LatticeValue>::DataFlowAnalysis(
        CreateStateFn create_state_fn,
        StateJoinFn state_join_fn,
        FlowFn flow_fn) {
        this->state_table = StateTable();
        this->create_state = create_state_fn;
        this->state_join = state_join_fn;
        this->flow = flow_fn;
    }

    template<typename State, typename LatticeValue>
    void DataFlowAnalysis<State, LatticeValue>::init_state_table(
        const Nodes &instructions,
        const Vars &vars,
        const Node &program_start,
        State first_state) {
        if (instructions.empty()) {
            throw std::runtime_error("No instructions exist for this program");
        }

        for (const auto &inst : instructions) {
            state_table.insert({inst, this->create_state(vars)});
        }

        state_table[program_start] = first_state;
    }

    template<typename State, typename LatticeValue>
    void DataFlowAnalysis<State, LatticeValue>::forward_worklist_algoritm(
        std::shared_ptr<ControlFlow> cfg, State first_state) {
        const auto instructions = cfg->get_instructions();
        const Vars vars = cfg->get_vars();

        std::deque<Node> worklist{cfg->get_program_entry()};
        this->init_state_table(
            instructions, vars, cfg->get_program_entry(), first_state);

        while (!worklist.empty()) {
            Node inst = worklist.front();
            worklist.pop_front();

            State out_state = flow(inst, state_table, cfg);
            state_table[inst] = out_state;

            for (Node succ : cfg->successors(inst)) {
                bool changed = this->state_join(state_table[succ], out_state);

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
        this->init_state_table(
            instructions, vars, cfg->get_program_exit(), first_state);

        while (!worklist.empty()) {
            Node inst = worklist.front();
            worklist.pop_front();

            Vars in_state = flow(inst, state_table, cfg);

            for (Node pred : cfg->predecessors(inst)) {
                Vars &succ_state = state_table[pred];
                bool changed = state_join(succ_state, in_state);

                if (changed) {
                    worklist.push_back(pred);
                }
            }
        }
    }

    // Requires the user to define the << operator for the State type
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
