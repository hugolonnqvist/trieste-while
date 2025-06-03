#pragma once
#include "../control_flow.hh"
#include "../internal.hh"

#define PRINT_WIDTH 15

namespace whilelang {
    using namespace trieste;

    // struct Implementation {};

    // The State represents the mapping of code information (typically
    // variables) to the abstract values (LatticeValue)
    template<typename State, typename LatticeValue, typename Impl>
    class DataFlowAnalysis {
      public:
        // Tracks a mapping from all program points to their corresponding state
        using StateTable = NodeMap<State>;

        // Creates a state which has not yet been reached.
        // Typically maps all variables to bottom
        // using CreateStateFn = std::function<State(const Vars &)>;
        //
        // // Joins the two state and stores the result in the first one.
        // Returns a
        // // bool stating if the resulting state is changed
        // using StateJoinFn = std::function<bool(State &, const State &)>;
        //
        // // Executes the flow function on an instruction
        // // returning the resulting state
        // using FlowFn = std::function<State(
        //     const Node &, StateTable &, std::shared_ptr<ControlFlow>)>;

        DataFlowAnalysis();

        State get_state(const Node &instruction) {
            return state_table[instruction];
        };

        void forward_worklist_algoritm(
            std::shared_ptr<ControlFlow> cfg, State first_state);

        void backward_worklist_algoritm(
            std::shared_ptr<ControlFlow> cfg, State first_state);

        // Requires that the << operator has been specified for the State type
        void log_state_table(std::shared_ptr<ControlFlow> cfg);

      private:
        StateTable state_table;

        void init_state_table(
            const Nodes &instructions,
            const Vars &vars,
            const Node &program_entry,
            State first_state);
    };

    template<typename State, typename LatticeValue, typename Impl>
    DataFlowAnalysis<State, LatticeValue, Impl>::DataFlowAnalysis() {
        this->state_table = StateTable();
    }

    template<typename State, typename LatticeValue, typename Impl>
    void DataFlowAnalysis<State, LatticeValue, Impl>::init_state_table(
        const Nodes &instructions,
        const Vars &vars,
        const Node &program_start,
        State first_state) {
        if (instructions.empty()) {
            throw std::runtime_error("No instructions exist for this program");
        }

        for (const auto &inst : instructions) {
            state_table.insert({inst, Impl::create_state(vars)});
        }

        state_table[program_start] = first_state;
    }

    template<typename State, typename LatticeValue, typename Impl>
    void DataFlowAnalysis<State, LatticeValue, Impl>::forward_worklist_algoritm(
        std::shared_ptr<ControlFlow> cfg, State first_state) {
        const auto instructions = cfg->get_instructions();
        const Vars vars = cfg->get_vars();

        std::deque<Node> worklist{cfg->get_program_entry()};
        this->init_state_table(
            instructions, vars, cfg->get_program_entry(), first_state);

        while (!worklist.empty()) {
            Node inst = worklist.front();
            worklist.pop_front();

            State out_state = Impl::flow(inst, state_table, cfg);
            state_table[inst] = out_state;

            for (Node succ : cfg->successors(inst)) {
                bool changed = Impl::state_join(state_table[succ], out_state);

                if (changed) {
                    worklist.push_back(succ);
                }
            }
        }
    }

    template<typename State, typename LatticeValue, typename Impl>
    void DataFlowAnalysis<State, LatticeValue, Impl>::backward_worklist_algoritm(
        std::shared_ptr<ControlFlow> cfg, State first_state) {
        const auto instructions = cfg->get_instructions();
        const Vars vars = cfg->get_vars();

        std::deque<Node> worklist{instructions.begin(), instructions.end()};
        this->init_state_table(
            instructions, vars, cfg->get_program_exit(), first_state);

        while (!worklist.empty()) {
            Node inst = worklist.front();
            worklist.pop_front();

            State in_state = Impl::flow(inst, state_table, cfg);

            for (Node pred : cfg->predecessors(inst)) {
                State &succ_state = state_table[pred];
                bool changed = Impl::state_join(succ_state, in_state);

                if (changed) {
                    worklist.push_back(pred);
                }
            }
        }
    }

    // Requires the user to define the << operator for the State type
    template<typename State, typename LatticeValue, typename Impl>
    void DataFlowAnalysis<State, LatticeValue, Impl>::log_state_table(
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
            str_builder << std::setw(PRINT_WIDTH) << i + 1
                        << state_table[instructions[i]] << '\n';
        }
        logging::Debug() << str_builder.str();
    }
}
