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
                (T(Stmt) << T(While)[While]) * T(Stmt)[Post] >>
                    [predecessor, instructions](Match &_) -> Node
                    {
                        auto node = get_first_basic_child(_(Post));
                        auto prev = _(While) / BExpr;
                        add_predecessor(predecessor, node, prev);

                        return NoChange;
                    },

                In(Semi) * (T(Stmt)[Prev] << !(T(While)))* T(Stmt)[Post] >>
                    [predecessor, instructions](Match &_) -> Node
                    {
                        auto node = get_first_basic_child(_(Post));
                        auto prev = get_last_basic_children(_(Prev));

                        add_predecessor(predecessor, node, prev);

                        return NoChange;
                    },
                
                // Special case for if statement
                In(Stmt) * T(If)[If] >>
                    [predecessor, instructions](Match &_) -> Node
                    {
                        auto b_expr = _(If) / BExpr;
                        auto then_stmt = _(If) / Then;
                        auto else_stmt = _(If) / Else;
                        
                        add_predecessor(predecessor, get_first_basic_child(then_stmt), b_expr);
                        add_predecessor(predecessor, get_first_basic_child(else_stmt), b_expr);
                        
                        return NoChange;
                    },

                // Special case for while statement
                In(Stmt) * T(While)[While]  >> 
                    [predecessor, instructions](Match &_) -> Node
                    {
                        auto b_expr = _(While) / BExpr;
                        auto body = _(While) / Do;

                        add_predecessor(predecessor, get_first_basic_child(body), b_expr);
                        add_predecessor(predecessor, b_expr, get_last_basic_children(body));
                        
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

            std::cout << "Instructions" << std::endl;
            for (auto &i : *instructions) {
                std::cout << i << std::endl;
            }

            return 0;

        });

        return init_flow_graph;
    }

}