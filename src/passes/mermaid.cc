#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {

    using namespace trieste;

    PassDef mermaid(const wf::Wellformed &wf) {
        auto ast_map = std::make_shared<NodeMap<NodeSet>>();
        auto id_map = std::make_shared<NodeMap<std::string>>();

        PassDef mermaid = {
            "mermaid",
            wf,
            dir::topdown | dir::once,
            {
                Any[Rhs] >> [=](Match &_) -> Node {
                    auto value = _(Rhs);
                    auto key = value->parent();
                    if (!key) {
                        return NoChange;
                    }

                    auto res = ast_map->find(key);

                    if (!ast_map->contains(value)) {
                        id_map->insert(
                            {value, std::string(value->fresh().view())});
                    }

                    if (res != ast_map->end()) {
                        // Append to nodemap entry if already exist
                        res->second.insert(value);
                    } else {
                        // Otherwise create new entry
                        ast_map->insert({key, {value}});
                        auto id = std::string(key->fresh().view());
                        id_map->insert({key, id});
                    }
                    return NoChange;
                },
            }};

        mermaid.post([=](Node) {
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
            for (const auto &[key, values] : *ast_map) {
                for (const auto &value : values) {
                    result << build_mermaid_node(key) << "-->"
                           << build_mermaid_node(value) << "\n";
                }
            }

            logging::Debug() << result.str();
            return 0;
        });

        return mermaid;
    }
}
