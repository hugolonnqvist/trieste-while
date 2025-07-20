#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    std::string get_label(Node node) {
        if (node != Label) {
            throw std::runtime_error("Node is not a label");
        }
        return std::string(node->location().view());
    }

    PassDef blockify() {
        PassDef blockify = {
            "blockify",
            blockify_wf,
            dir::bottomup | dir::once,
            {
                T(FunDef) <<
                    (T(FunId)[FunId] *
                     T(ParamList)[ParamList] *
                     T(Idents)[Idents] *
                     (T(Stmt) << T(Block)[Body])) >>
                    [](Match &_) -> Node {
                        auto fun_body = _(Body);
                        if (fun_body->size() == 0) {
                            return Error << (ErrorAst << _(FunId))
                                         << (ErrorMsg ^ "Empty function body");
                        } else if (fun_body->at(0) / Stmt != Label) {
                            return Error << (ErrorAst << _(FunId))
                                         << (ErrorMsg ^ "Function body must start with a label");
                        }

                        // Only keep labels that are targeted by jumps
                        std::set<std::string> targeted_labels;
                        targeted_labels.insert(get_label(fun_body->at(0) / Stmt));

                        Node blocks = Blocks;
                        Node block = Block;
                        Node body = Body;
                        for (auto child : *fun_body) {
                            auto stmt = child / Stmt;
                            if (stmt != Label && block->size() == 0) {
                                // Unreachable statement, skip it
                                continue;
                            }

                            if (stmt == Label) {
                                if (block->size() == 0) {
                                    // First label in the block
                                    if (targeted_labels.count(get_label(stmt)) == 0)
                                        continue; // Label is not targeted, skip it
                                    block << stmt;
                                } else {
                                    // Fallthrough
                                    targeted_labels.insert(get_label(stmt));
                                    block << body << (Jump << stmt->clone());
                                    blocks << block;
                                    block = Block << stmt;
                                    body = Body;
                                }
                            } else if (stmt->type().in({Cond, Jump, Return})) {
                                if (stmt == Jump) {
                                    targeted_labels.insert(get_label(stmt / Label));
                                } else if (stmt == Cond) {
                                    targeted_labels.insert(get_label(stmt / Then));
                                    targeted_labels.insert(get_label(stmt / Else));
                                }
                                block << body << stmt;
                                blocks << block;
                                block = Block;
                                body = Body;
                            } else {
                                body << child;
                            }
                        }

                        if (block->size() > 0)
                            return Error << (ErrorAst << _(FunId))
                                         << (ErrorMsg ^ "Control reaches end of function");

                        return FunDef << _(FunId) << _(ParamList) << _(Idents) << blocks;
                    },
            }
        };

        return blockify;
    }
}
