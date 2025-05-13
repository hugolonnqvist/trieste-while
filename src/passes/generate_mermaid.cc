#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {

    using namespace trieste;

    PassDef generate_mermaid(const wf::Wellformed &wf) {
        auto children_map = std::make_shared<NodeMap<NodeSet>>();
        auto id_map = std::make_shared<NodeMap<std::string>>();

        PassDef generate_mermaid = {
            "mermaid",
            wf,
            dir::topdown | dir::once,
            {
                Any[Rhs] >> [=](Match &_) -> Node {
                    auto child = _(Rhs);
                    auto parent = child->parent();

                    if (!parent) {
                        return NoChange;
                    }

                    if (!children_map->contains(child)) {
                        id_map->insert(
                            {child, std::string(child->fresh().view())});
                    }

                    auto res = children_map->find(parent);

                    if (res != children_map->end()) {
                        // Append to nodemap entry if already exist
                        res->second.insert(child);
                    } else {
                        // Otherwise create new entry
                        children_map->insert({parent, {child}});

                        auto id = std::string(parent->fresh().view());
                        id_map->insert({parent, id});
                    }
                    return NoChange;
                },
            }};

        generate_mermaid.post([=](Node) {
            auto get_id = [=](Node n) -> std::string {
                auto res = id_map->find(n);
                if (res == id_map->end()) {
                    throw std::runtime_error("Invalid node");
                } else {
                    return res->second;
                }
            };

            auto get_str = [](Node n) -> std::string {
                if (n == Int) {
                    return "int : " + get_identifier(n);
                } else if (n == Add) {
                    return "add";
                } else if (n == Sub) {
                    return "sub";
                } else if (n == Mul) {
                    return "mul";
                } else if (n == Ident) {
                    return "ident : " + get_identifier(n);
                }
                return std::string(n->type().str());
            };

            auto build_mermaid_node = [=](Node n) -> std::string {
                auto res = std::stringstream();
                res << get_id(n) << "[" << get_str(n) << "]";
                return res.str();
            };

            auto result = std::stringstream();
            for (const auto &[key, values] : *children_map) {
                for (const auto &value : values) {
                    result << build_mermaid_node(key) << "-->"
                           << build_mermaid_node(value) << "\n";
                }
            }

            logging::Debug() << result.str();
            return 0;
        });

        return generate_mermaid;
    }
}
