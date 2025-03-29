#pragma once
#include "lang.hh"

namespace whilelang
{
  using namespace trieste;

  Parse parser();
  PassDef expressions();
  PassDef statements();
  PassDef check_refs();
  PassDef init_flow_graph();

  inline const auto parse_token =
    Skip |
    If | Then | Else |
    While | Do |
    Output |
    Int | Ident | Input |
    True | False | Not |
    Paren | Brace
    ;

  inline const auto grouping_construct =
    Group | Semi | Paren | Brace |
    Add | Sub | Mul |
    LT | Equals |
    And | Or |
    Assign
    ;

  inline const wf::Wellformed parse_wf =
    (Top <<= File)
    | (File   <<= ~grouping_construct)
    | (Semi   <<= (grouping_construct - Semi)++[1])
    | (If     <<= ~grouping_construct)
    | (Then   <<= ~grouping_construct)
    | (Else   <<= ~grouping_construct)
    | (While  <<= ~grouping_construct)
    | (Do     <<= ~grouping_construct)
    | (Output <<= ~grouping_construct)
    | (Assign <<= (grouping_construct - Assign)++[1])
    | (Add    <<= (grouping_construct - Add)++[1])
    | (Sub    <<= (grouping_construct - Sub)++[1])
    | (Mul    <<= (grouping_construct - Mul - Add - Sub)++[1])
    | (LT     <<= (grouping_construct - LT)++[1])
    | (Equals <<= (grouping_construct - Equals)++[1])
    | (And    <<= (grouping_construct - And - Or)++[1])
    | (Or     <<= (grouping_construct - Or)++[1])
    | (Paren  <<= ~grouping_construct)
    | (Brace  <<= ~grouping_construct)
    | (Group  <<= parse_token++)
    ;

  inline const auto expressions_parse_token = parse_token - Not - True - False - Int - Ident - Input;
  inline const auto expressions_grouping_construct = (grouping_construct - Add - Sub - Mul - LT - Equals - And - Or) | AExpr | BExpr;

  inline const wf::Wellformed expressions_wf =
    parse_wf
    | (File   <<= ~expressions_grouping_construct)
    | (AExpr  <<= (Int | Ident | Mul | Add | Sub | Input))
    | (BExpr  <<= (True | False | Not | Equals | LT | And | Or))
    | (Add    <<= AExpr++[2])
    | (Sub    <<= AExpr++[2])
    | (Mul    <<= AExpr++[2])
    | (LT     <<= AExpr * AExpr)
    | (Equals <<= AExpr * AExpr)
    | (And    <<= BExpr++[2])
    | (Or     <<= BExpr++[2])
    | (Not    <<= BExpr)
    | (Semi   <<= (expressions_grouping_construct - Semi)++[1])
    | (If     <<= ~expressions_grouping_construct)
    | (Then   <<= ~expressions_grouping_construct)
    | (Else   <<= ~expressions_grouping_construct)
    | (While  <<= ~expressions_grouping_construct)
    | (Do     <<= ~expressions_grouping_construct)
    | (Output <<= ~expressions_grouping_construct)
    | (Assign <<= (expressions_grouping_construct - Assign)++[1])
    | (Paren  <<= expressions_grouping_construct)
    | (Brace  <<= ~expressions_grouping_construct)
    | (Group  <<= expressions_parse_token++)
    ;

    inline const wf::Wellformed statements_wf =
      (expressions_wf - Group - Paren - Do - Then - Else)
      | (Top <<= ~Program)
      | (Program <<= Stmt)
      | (Stmt <<= (Skip | Assign | While | If | Output | Semi))
      | (If <<= BExpr * (Then >>= Stmt) * (Else >>= Stmt))
      | (While <<= BExpr * (Do >>= Stmt))
      | (Assign <<= Ident * AExpr)[Ident]
      | (Output <<= AExpr)
      | (Brace <<= expressions_grouping_construct)
      | (Semi <<= Stmt++[1])
      ;

    inline const wf::Wellformed eval_wf =
      statements_wf - Program;

}
