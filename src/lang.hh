#pragma once
#include <trieste/trieste.h>

namespace whilelang {
    using namespace trieste;

    Reader reader(
        std::shared_ptr<std::map<std::string, std::string>> vars_map,
        bool run_stats,
        bool run_mermaid);
    Rewriter interpret();
    Rewriter optimization_analysis(bool run_zero_analysis);
    Rewriter compiler();

    // Program
    inline const auto Program = TokenDef("while-program");

    // Function
    inline const auto FunDef =
        TokenDef("while-function_declaration", flag::symtab | flag::defbeforeuse);
    inline const auto FunId = TokenDef("while-function_identifier", flag::print);
    inline const auto ParamList = TokenDef("while-param_list");
    inline const auto Param = TokenDef("while-param", flag::lookup | flag::shadowing);
    inline const auto Body = TokenDef("while-body");
    inline const auto Var = TokenDef("while-var", flag::lookup | flag::shadowing);
    inline const auto Return = TokenDef("while-return");

    inline const auto FunCall = TokenDef("while-function_call");
    inline const auto ArgList = TokenDef("while-arg_list");
    inline const auto Arg = TokenDef("while-arg");
    inline const auto Comma = TokenDef("while-comma");

    // Statements
    inline const auto Assign = TokenDef("while-:=");

    inline const auto Skip = TokenDef("while-skip");

    inline const auto Semi = TokenDef("while-;");

    inline const auto If = TokenDef("while-if");
    inline const auto Then = TokenDef("while-then");
    inline const auto Else = TokenDef("while-else");

    inline const auto While = TokenDef("while-while");
    inline const auto Do = TokenDef("while-do");

    inline const auto Output = TokenDef("while-output");

    // Constants
    inline const auto Int = TokenDef("while-int", flag::print);
    inline const auto True = TokenDef("while-true");
    inline const auto False = TokenDef("while-false");

    // Input
    inline const auto Input = TokenDef("while-input");

    // Arithmetic operators
    inline const auto Add = TokenDef("while-+");
    inline const auto Sub = TokenDef("while--");
    inline const auto Mul = TokenDef("while-*");

    // Boolean operators
    inline const auto And = TokenDef("while-and");
    inline const auto Or = TokenDef("while-or");
    inline const auto Not = TokenDef("while-not");

    // Comparison expressions
    inline const auto LT = TokenDef("while-<");
    inline const auto Equals = TokenDef("while-=");

    // Identifiers
    inline const auto Ident = TokenDef("while-ident", flag::print);

    // Grouping tokens
    inline const auto Paren = TokenDef("while-paren");
    inline const auto Brace = TokenDef("while-brace");

    inline const auto Block =
        TokenDef("while-block", flag::symtab | flag::defbeforeuse);

    inline const auto Stmt = TokenDef("while-stmt");
    inline const auto Expr = TokenDef("while-expr");
    inline const auto AExpr = TokenDef("while-aexpr");
    inline const auto BExpr = TokenDef("while-bexpr");

    // Convenience
    inline const auto Lhs = TokenDef("while-lhs");
    inline const auto Rhs = TokenDef("while-rhs");
    inline const auto Op = TokenDef("while-op");
    inline const auto Prev = TokenDef("while-prev");
    inline const auto Post = TokenDef("while-post");
    inline const auto Inst = TokenDef("while-inst");
    inline const auto Idents = TokenDef("while-idents");

    // Evaluation
    inline const auto Eval = TokenDef("while-eval");

    // Normalization
    inline const auto Normalize = TokenDef("while-normalize");
    inline const auto Atom = TokenDef("while-atom");
    inline const auto BAtom = TokenDef("while-batom");
    inline const auto Instructions = TokenDef("while-instructions");

    // 3 address code
    inline const auto Label = TokenDef("while-label", flag::print);
    inline const auto Jump = TokenDef("while-jump");
    inline const auto Cond = TokenDef("while-cond");
    inline const auto Blocks = TokenDef("while-blocks");

    // Compilation
    inline const auto Compile = TokenDef("while-compile");
}
