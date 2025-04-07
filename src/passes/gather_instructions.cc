#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    // clang-format off
    PassDef gather_instructions(std::shared_ptr<ControlFlow> control_flow) {
		PassDef gather_instructions = {
            "gather_instructions",
            normalization_wf,
            dir::topdown | dir::once,
            {
                T(Assign, Skip, Output)[Inst] >>
                    [control_flow](Match &_) -> Node
                    {
                        Node inst = _(Inst);
						control_flow->add_instruction(inst);

                        // Gather variables
                        if (inst->type() == Assign) {
                            auto ident = inst / Ident;
							control_flow->add_var(ident);
                        }

                        return NoChange;
                    },

                In(While, If) * T(BExpr)[Inst] >>
                    [control_flow](Match &_) -> Node
                    {
                        control_flow->add_instruction(_(Inst));
                        return NoChange;
                    },
			}
		};

		gather_instructions.post([control_flow](Node) {
			if (control_flow->get_instructions().empty()) {
				throw std::runtime_error("Unexpected, missing instructions");
			}
			
			return 0;	
		});
		return gather_instructions;
    }
}
