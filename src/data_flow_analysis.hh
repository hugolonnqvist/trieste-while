#pragma once
#include "control_flow.hh"
#include "lang.hh"
#include "utils.hh"

namespace whilelang {
    using namespace trieste;

    template<typename LatticeValue>
    class DataFlowAnalysis {
      public:
        using State = std::unordered_map<std::string, LatticeValue>;
        using JoinFn = std::function<LatticeValue(LatticeValue, LatticeValue)>;
        using FlowFn = std::function<State(
            Node,
            std::unordered_map<Node, State>,
            std::shared_ptr<ControlFlow>)>;

        DataFlowAnalysis(JoinFn join_fn, FlowFn flow_fn);

        State create_state(Vars vars, LatticeValue value);

        LatticeValue get_lattice_value(Node inst, std::string var);

        State join(const State x, const State y);
        bool join_mut(State &x, const State y);

        void set_state(const Node inst, LatticeValue value);

        void forward_worklist_algoritm(
            std::shared_ptr<ControlFlow> control_flow,
            LatticeValue first_state,
            LatticeValue bottom);

        void log_state_table(const Nodes instructions);

      private:
        std::unordered_map<Node, State> state_table;
        JoinFn join_fn;
        FlowFn flow_fn;

        bool state_equals(State x, State y);

        void init(
            const Nodes instructions,
            const Vars vars,
            const Node program_entry,
            LatticeValue first_state,
            LatticeValue bottom);
    };

    template<typename LatticeValue>
    typename DataFlowAnalysis<LatticeValue>::State
    DataFlowAnalysis<LatticeValue>::create_state(
        Vars vars, LatticeValue value) {
        State state = State();
        for (auto var : vars) {
            state[var] = value;
        }
        return state;
    }

    template<typename LatticeValue>
    DataFlowAnalysis<LatticeValue>::DataFlowAnalysis(JoinFn join, FlowFn flow) {
        this->state_table = std::unordered_map<Node, State>();
        this->join_fn = join;
        this->flow_fn = flow;
    }

    template<typename LatticeValue>
    void DataFlowAnalysis<LatticeValue>::init(
        const Nodes instructions,
        const Vars vars,
        const Node program_entry,
        LatticeValue first_state,
        LatticeValue bottom) {
        if (instructions.empty()) {
            throw std::runtime_error("No instructions exist for this program");
        }

        for (auto inst : instructions) {
            state_table.insert({inst, create_state(vars, bottom)});
        }

        set_state(program_entry, first_state);
    }

    template<typename LatticeValue>
    LatticeValue DataFlowAnalysis<LatticeValue>::get_lattice_value(
        Node inst, std::string var) {
        return state_table[inst][var];
    }

    template<typename LatticeValue>
    typename DataFlowAnalysis<LatticeValue>::State
    DataFlowAnalysis<LatticeValue>::join(const State x, const State y) {
        State result_state = x;

        for (const auto &[key, val_y] : y) {
            auto res = result_state.find(key);
            if (res != result_state.end()) {
                auto val_x = res->second;
                result_state[key] = join_fn(val_x, val_y);
            } else {
                throw std::runtime_error("State do not have the same keys");
            }
        }
        return result_state;
    }

    template<typename LatticeValue>
    bool DataFlowAnalysis<LatticeValue>::join_mut(State &x, const State y) {
        bool changed = false;
        for (const auto &[key, val_y] : y) {
            auto res = x.find(key);
            if (res != x.end()) {
                auto join_res = join_fn(res->second, val_y);
                if (!(res->second == join_res)) {
                    x[key] = join_res;
                    changed = true;
                }
            } else {
                throw std::runtime_error("State do not have the same keys");
            }
        }
        return changed;
    }

    template<typename LatticeValue>
    void DataFlowAnalysis<LatticeValue>::set_state(
        const Node inst, LatticeValue value) {
        State new_state = State();

        for (auto &[_, old_st] : state_table[inst]) {
            old_st = value;
        }
    }

    template<typename LatticeValue>
    bool DataFlowAnalysis<LatticeValue>::state_equals(State x, State y) {
        if (x.size() != y.size()) {
            throw std::runtime_error("State sizes do not match");
        }

        for (auto it1 = x.begin(), it2 = y.begin();
             it1 != x.end() && it2 != y.end();
             ++it1, ++it2) {
            auto [key1, val1] = *it1;
            auto [key2, val2] = *it2;

            if (key1 != key2) {
                throw std::runtime_error("State keys do not match");
            }

            if (val1 != val2) {
                return false;
            }
        }
        return true;
    }

    template<typename LatticeValue>
    void DataFlowAnalysis<LatticeValue>::forward_worklist_algoritm(
        std::shared_ptr<ControlFlow> cfg,
        LatticeValue first_state,
        LatticeValue bottom) {
        const auto instructions = cfg->get_instructions();
        const Vars vars = cfg->get_vars();

        std::deque<Node> worklist{cfg->get_program_entry()};
        this->init(
            instructions, vars, cfg->get_program_entry(), first_state, bottom);

        while (!worklist.empty()) {
            Node inst = worklist.front();
            worklist.pop_front();

            State out_state = flow_fn(inst, state_table, cfg);
            state_table[inst] = out_state;

            for (Node succ : cfg->successors(inst)) {
                State &succ_state = state_table[succ];
                bool changed = this->join_mut(succ_state, out_state);

                if (changed) {
                    worklist.push_back(succ);
                }
            }
        }
    }

    template<typename LatticeValue>
    void
    DataFlowAnalysis<LatticeValue>::log_state_table(const Nodes instructions) {
        const int width = 15;
        const int number_of_vars = state_table[instructions[0]].size();
        std::stringstream str_builder;

        str_builder << std::left << std::setw(width) << "";
        for (const auto &[key, _] : state_table[instructions[0]]) {
            str_builder << std::setw(width) << key;
        }

        str_builder << std::endl;
        str_builder << std::string(width * (number_of_vars + 1), '-')
                    << std::endl;

        for (size_t i = 0; i < instructions.size(); i++) {
            str_builder << std::setw(width) << i + 1;

            auto curr = instructions[i];
            while (curr != FunDef) {
                curr = curr->parent();
            }
            auto fun_str = get_identifier((curr / FunId) / Ident);
            for (const auto &[var, value] : state_table[instructions[i]]) {
                if (var.ends_with(fun_str)) {
                    str_builder << std::setw(width) << value;
                } else {
                    str_builder << std::setw(width) << "_";
                }
            }
            str_builder << '\n';
        }
        logging::Debug() << str_builder.str();
    }
}
