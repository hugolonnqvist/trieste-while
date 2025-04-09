#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    // clang-format off
    PassDef dead_code_elimination(std::shared_ptr<ControlFlow> control_flow) {
		auto analysis = std::make_shared<DataFlowAnalysis>();

		PassDef dead_code_elimination = {
            "dead_code_elimination",
            statements_wf,
            dir::bottomup | dir::once,
            {
			}
		};
		

		dead_code_elimination.pre([=](Node) {
			// Live ness	
            control_flow->log_instructions();
            log_cp_state_table(instructions, analysis->get_state_table());


			return 0;
		});
		
		return dead_code_elimination;
    }
}
