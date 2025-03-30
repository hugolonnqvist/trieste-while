#include "../internal.hh"
#include "../utils.hh"

namespace whilelang
{
    using namespace trieste;

    PassDef init_flow_graph()
    {
        auto predecessor = std::make_shared<NodeMap<std::set<Node>>>();
        auto instructions = std::make_shared<std::vector<Node>>();

        PassDef init_flow_graph =  {
            "init_flow_graph",
            statements_wf,
            dir::topdown | dir::once,
            {
                // Control flow inside of While
                T(While)[While] >>
                    [predecessor, instructions](Match &_) -> Node
                    {
                        auto b_expr = _(While) / BExpr;
                        auto body = _(While) / Do;

                        add_predecessor(predecessor, b_expr, get_last_basic_children(body));
                        add_predecessor(predecessor, get_first_basic_child(body), b_expr);
                        instructions->push_back(b_expr);

                        return NoChange;
                    },

                // Control flow inside of If
                T(If)[If] >> 
                    [predecessor, instructions](Match &_) -> Node
                    {
                        auto b_expr = _(If) / BExpr;
                        auto then_stmt = _(If) / Then;
                        auto else_stmt = _(If) / Else;

                        add_predecessor(predecessor, get_first_basic_child(then_stmt), b_expr);
                        add_predecessor(predecessor, get_first_basic_child(else_stmt), b_expr);

                        return NoChange;
                    },
                
                // Special case of While as predecessor
                (T(Stmt)[Prev] << T(While)[While])* T(Stmt)[Post] >>
                    [predecessor, instructions](Match &_) -> Node
                    {
                        auto node = get_first_basic_child(_(Post));
                        auto prev = _(While) / BExpr;

                        add_predecessor(predecessor, node, prev);

                        return NoChange;
                    },

                // General case of a sequence of statements
                In(Semi) * (T(Stmt)[Prev] << !(T(While))) * T(Stmt)[Post] >>
                    [predecessor, instructions](Match &_) -> Node
                    {
                        auto node = get_first_basic_child(_(Post));
                        auto prev = get_last_basic_children(_(Prev));

                        add_predecessor(predecessor, node, prev);

                        return NoChange;
                    },

        }};

        init_flow_graph.post([predecessor, instructions](Node)  {
            
            for (auto it = predecessor->begin(); it != predecessor->end(); it++) {
                std::cout << it->first << " has predecessors: ";
                for (auto &p : it->second) {
                    std::cout << p << " ";
                }
                std::cout << std::endl;
            }

            return 0;

        });

        return init_flow_graph;
    }
    PassDef gather_instructions() {

        auto instructions = std::make_shared<std::vector<Node>>();

        PassDef gather_instructions = {
            "gather_instructions",
            statements_wf,
            dir::topdown | dir::once,
            {
                T(Assign, Skip)[Inst] >>
                    [instructions](Match &_) -> Node
                    {
                        instructions->push_back(_(Inst));
                        return NoChange;
                    },
                In(While, If) * T(BExpr)[Inst] >>
                    [instructions](Match &_) -> Node
                    {
                        instructions->push_back(_(Inst));
                        return NoChange;
                    },
            }
        };

        gather_instructions.post([instructions](Node)  {
            std::cout << "Instructions" << std::endl;
            for (auto &i : *instructions) {
                std::cout << i << std::endl;
            }

            return 0;

        });

        return gather_instructions;
    }
}