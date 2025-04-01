#pragma once
#include <trieste/trieste.h>

namespace whilelang
{
  using namespace trieste;

  Reader reader();

  // Program
  inline const auto Program = TokenDef("program", flag::symtab | flag::defbeforeuse);

  // Statements
  inline const auto Assign = TokenDef(":=", flag::lookup);

  inline const auto Skip = TokenDef("skip");

  inline const auto Semi = TokenDef(";");

  inline const auto If = TokenDef("if");
  inline const auto Then = TokenDef("then");
  inline const auto Else = TokenDef("else");

  inline const auto While = TokenDef("while");
  inline const auto Do = TokenDef("do");

  inline const auto Output = TokenDef("output");

  // Constants
  inline const auto Int = TokenDef("int", flag::print);
  inline const auto True = TokenDef("true");
  inline const auto False = TokenDef("false");

  // Input
  inline const auto Input = TokenDef("input");

  // Arithmetic operators
  inline const auto Add = TokenDef("+");
  inline const auto Sub = TokenDef("-");
  inline const auto Mul = TokenDef("*");

  // Boolean operators
  inline const auto And = TokenDef("and");
  inline const auto Or = TokenDef("or");
  inline const auto Not = TokenDef("not");

  // Comparison expressions
  inline const auto LT = TokenDef("<");
  inline const auto Equals = TokenDef("=");

  // Identifiers
  inline const auto Ident = TokenDef("ident", flag::print);

  // Grouping tokens
  inline const auto Paren = TokenDef("paren");
  inline const auto Brace = TokenDef("brace");

  inline const auto Stmt = TokenDef("stmt");
  inline const auto Expr = TokenDef("expr"); // Not yet decided if arithmetic or boolean
  inline const auto AExpr = TokenDef("aexpr");
  inline const auto BExpr = TokenDef("bexpr");

  // Convenience
  inline const auto Lhs = TokenDef("lhs");
  inline const auto Rhs = TokenDef("rhs");
  inline const auto Op = TokenDef("op");
  inline const auto Prev = TokenDef("prev");
  inline const auto Post = TokenDef("post");
  inline const auto Inst = TokenDef("inst");

  // Static analysis
  inline const auto TTop = TokenDef("t_top");
  inline const auto TBottom = TokenDef("t_bottom");

  // Zero analysis
  inline const auto TZero = TokenDef("t_zero");
  inline const auto TNonZero = TokenDef("t_non_zero");
}
