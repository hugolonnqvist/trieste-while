#include "data_flow_analysis.hh"

namespace whilelang {
    using namespace trieste;

    struct SetLattice {
        NodeMap<Vars> in_set;
        NodeMap<Vars> out_set;

        SetLattice() {
            this->in_set = NodeMap<Vars>();
            this->out_set = NodeMap<Vars>();
        }

        void init(const Nodes instructions) {
            for (auto inst : instructions) {
                in_set.insert({inst, {}});
                out_set.insert({inst, {}});
            }
        }

        void log(const Nodes instructions) {
            logging::Debug() << "Out set: ";
            log_power_lattice(instructions, out_set);
            logging::Debug() << "In set: ";
            log_power_lattice(instructions, in_set);
        }

       private:
        void log_power_lattice(const Nodes instructions,
                               NodeMap<Vars> lattice) {
            std::stringstream str;
            for (size_t i = 0; i < instructions.size(); i++) {
                auto inst = instructions[i];
                str << "Instruction: " << i + 1
                    << " has the follwing live variables: \n";
                for (auto var : lattice[inst]) {
                    str << " " << var;
                }
                str << "\n";
            }
            logging::Debug() << str.str();
        }
    };
}
