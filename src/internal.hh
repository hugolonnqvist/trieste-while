#pragma once
#include "control_flow.hh"
#include "data_flow_analysis.hh"
#include "lang.hh"
#include "utils.hh"

namespace whilelang {
    using namespace trieste;

    Parse parser();
    PassDef expressions();
    PassDef statements();
    PassDef check_refs();
    PassDef eval();
    PassDef normalization();
    PassDef gather_instructions(std::shared_ptr<ControlFlow> control_flow);
    PassDef gather_flow_graph(std::shared_ptr<ControlFlow> control_flow);
    PassDef z_analysis(std::shared_ptr<ControlFlow> control_flow);

    PassDef constant_folding(std::shared_ptr<ControlFlow> control_flow);
    PassDef dead_code_elimination(
        std::shared_ptr<ControlFlow> control_flow, bool &changes);
    PassDef dead_code_cleanup(bool &changes);

    // clang-format off
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
		| (AExpr  <<= (Expr >>= (Int | Ident | Mul | Add | Sub | Input)))
		| (BExpr  <<= (Expr >>= (True | False | Not | Equals | LT | And | Or)))
		| (Add    <<= AExpr++[2])
		| (Sub    <<= AExpr++[2])
		| (Mul    <<= AExpr++[2])
		| (LT     <<= (Lhs >>= AExpr) * (Rhs >>= AExpr))
		| (Equals <<= (Lhs >>= AExpr) * (Rhs >>= AExpr))
		| (And    <<= BExpr++[2])
		| (Or     <<= BExpr++[2])
		| (Not    <<= (Expr >>= BExpr))
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
		| (Top <<= Program)
		| (Program <<= Stmt)
		| (Stmt <<= (Stmt >>= (Skip | Assign | While | If | Output | Semi)))
		| (If <<= BExpr * (Then >>= Stmt) * (Else >>= Stmt))
		| (While <<= BExpr * (Do >>= Stmt))
		| (Assign <<= Ident * (Rhs >>= AExpr))[Ident]
		| (Output <<= AExpr)
		| (Brace <<= expressions_grouping_construct)
		| (Semi <<= Stmt++[1])
		;

	inline const wf::Wellformed eval_wf =
		statements_wf - Program;

	inline const wf::Wellformed normalization_wf = 
		statements_wf
		| (Program <<= ~Stmt)
		| (AExpr <<= (Expr >>= (Atom | Add | Sub | Mul)))
		| (Atom <<= (Expr >>= (Int | Ident | Input)))
		| (Add <<= (Lhs >>= Atom) * (Rhs >>= Atom))
		| (Sub <<= (Lhs >>= Atom) * (Rhs >>= Atom))
		| (Mul <<= (Lhs >>= Atom) * (Rhs >>= Atom))
		| (LT <<= (Lhs >>= Atom) * (Rhs >>= Atom))
		| (Equals <<= (Lhs >>= Atom) * (Rhs >>= Atom))
		| (Output <<= Atom)
		;
}
