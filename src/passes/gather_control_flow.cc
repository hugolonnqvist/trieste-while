#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef gather_instructions(std::shared_ptr<ControlFlow> control_flow) {
        PassDef gather_instructions = {
            "gather_instructions",
            normalization_wf,
            dir::topdown | dir::once,
            {
                T(Assign, Skip, Output)[Inst] >>
                    [control_flow](Match &_) -> Node {
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
                    [control_flow](Match &_) -> Node {
                    control_flow->add_instruction(_(Inst));
                    return NoChange;
                },
            }};

        gather_instructions.post([control_flow](Node) {
            if (control_flow->get_instructions().empty()) {
                throw std::runtime_error("Unexpected, missing instructions");
            }

            return 0;
        });
        return gather_instructions;
    }

    PassDef gather_flow_graph(std::shared_ptr<ControlFlow> control_flow) {
        PassDef gather_flow_graph = {
            "gather_flow_graph",
            normalization_wf,
            dir::topdown | dir::once,
            {
                // Control flow inside of While
                T(While)[While] >> [control_flow](Match &_) -> Node {
                    auto b_expr = _(While) / BExpr;
                    auto body = _(While) / Do;

                    control_flow->add_predecessor(
                        b_expr, get_last_basic_children(body));
                    control_flow->add_predecessor(
                        get_first_basic_child(body), b_expr);

                    control_flow->add_successor(
                        b_expr, get_first_basic_child(body));
                    control_flow->add_successor(
                        get_last_basic_children(body), b_expr);

                    return NoChange;
                },

                // Control flow inside of If
                T(If)[If] >> [control_flow](Match &_) -> Node {
                    auto b_expr = _(If) / BExpr;
                    auto then_stmt = _(If) / Then;
                    auto else_stmt = _(If) / Else;

                    control_flow->add_predecessor(
                        get_first_basic_child(then_stmt), b_expr);
                    control_flow->add_predecessor(
                        get_first_basic_child(else_stmt), b_expr);

                    control_flow->add_successor(
                        b_expr, get_first_basic_child(then_stmt));
                    control_flow->add_successor(
                        b_expr, get_first_basic_child(else_stmt));

                    return NoChange;
                },

                // General case of a sequence of statements
                In(Semi) * T(Stmt)[Prev] * T(Stmt)[Post] >>
                    [control_flow](Match &_) -> Node {
                    auto prev = _(Prev);
                    auto post = _(Post);

                    control_flow->add_predecessor(
                        get_first_basic_child(post),
                        get_last_basic_children(prev));
                    control_flow->add_successor(
                        get_last_basic_children(prev),
                        get_first_basic_child(post));

                    return NoChange;
                },
            }};
        return gather_flow_graph;
    }
}
