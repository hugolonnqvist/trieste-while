#pragma once
#include "control_flow.hh"
#include "lang.hh"

namespace whilelang {
    using namespace trieste;

    // Parsing
    Parse parser();
    PassDef functions();
    PassDef expressions();
    PassDef statements();
    PassDef check_refs();
    PassDef unique_variables(
        std::shared_ptr<std::map<std::string, std::string>> vars_map);

    // Evaluation
    PassDef eval();

    // For performance testing
    PassDef gather_stats();

    // Gathering AST in mermaid
    PassDef generate_mermaid(const wf::Wellformed &wf);

    // Pre static analysis
    PassDef normalization();
    PassDef gather_functions(std::shared_ptr<ControlFlow> cfg);
    PassDef gather_instructions(std::shared_ptr<ControlFlow> cfg);
    PassDef gather_flow_graph(std::shared_ptr<ControlFlow> cfg);

    // Static analysis
    PassDef z_analysis(std::shared_ptr<ControlFlow> cfg);
    PassDef constant_folding(std::shared_ptr<ControlFlow> cfg);
    PassDef dead_code_elimination(std::shared_ptr<ControlFlow> cfg);
    PassDef dead_code_cleanup();

    // clang-format off
	inline const auto parse_token =
		Skip |
		If | Then | Else |
		While | Do |
		FunDef | Return | FunCall | Var |
		Output |
		Int | Ident | Input |
		True | False | Not |
		Paren | Brace
		;

	inline const auto grouping_construct =
		Group | Semi | Paren | Brace | Comma | 
		Add | Sub | Mul |
		LT | Equals |
		And | Or |
		Assign 
		;

	inline const wf::Wellformed parse_wf =
		(Top <<= File)
		| (File		<<= grouping_construct++)
		| (FunDef	<<= ~grouping_construct)
		| (Return	<<= ~grouping_construct)
		| (Var	    <<= ~grouping_construct)
		| (Semi		<<= (grouping_construct - Semi)++[1])
		| (Comma	<<= (grouping_construct - Comma)++[1])
		| (If		<<= ~grouping_construct)
		| (Then		<<= ~grouping_construct)
		| (Else		<<= ~grouping_construct)
		| (While	<<= ~grouping_construct)
		| (Do		<<= ~grouping_construct)
		| (Output	<<= ~grouping_construct)
		| (Assign	<<= (grouping_construct - Assign)++[1])
		| (Add		<<= (grouping_construct - Add)++[1])
		| (Sub		<<= (grouping_construct - Sub)++[1])
		| (Mul		<<= (grouping_construct - Mul - Add - Sub)++[1])
		| (LT		<<= (grouping_construct - LT)++[1])
		| (Equals	<<= (grouping_construct - Equals)++[1])
		| (And		<<= (grouping_construct - And - Or)++[1])
		| (Or		<<= (grouping_construct - Or)++[1])
		| (Paren	<<= ~grouping_construct)
		| (Brace	<<= ~grouping_construct)
		| (Group	<<= parse_token++)
		;

	inline const wf::Wellformed functions_wf = 
		(parse_wf - File)
		| (Top <<= Program)
		| (Program <<= FunDef++[1])
		| (FunDef <<= FunId * ParamList * Body)
		| (ParamList <<= Param++)
		| (Param <<= Ident)[Ident]
		| (FunId <<= Ident)
		| (Body <<= ~grouping_construct)
		| (Var <<= Ident)[Ident]
		;

	inline const auto expressions_parse_token = parse_token - Not - True - False - Int - Ident - Input;
	inline const auto expressions_grouping_construct = (grouping_construct - Add - Sub - Mul - LT - Equals - And - Or) | AExpr | BExpr;

	inline const wf::Wellformed expressions_wf =
		functions_wf
		| (AExpr  <<= (Expr >>= (Int | Ident | Mul | Add | Sub | Input | FunCall)))
		| (BExpr  <<= (Expr >>= (True | False | Not | Equals | LT | And | Or)))
		| (FunCall <<= FunId * ArgList)
		| (ArgList <<= Arg++)
		| (Arg <<= AExpr)
		| (Body	  <<= ~expressions_grouping_construct)
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
		| (Return <<= ~expressions_grouping_construct)
		| (Assign <<= (expressions_grouping_construct - Assign)++[1])
		| (Paren  <<= expressions_grouping_construct)
		| (Brace  <<= ~expressions_grouping_construct)
		| (Group  <<= expressions_parse_token++)
		;

    inline const wf::Wellformed statements_wf =
		(expressions_wf - Group - Paren - Do - Then - Else - Body)
		| (FunDef <<= FunId * ParamList * (Body >>= Stmt))
		| (Stmt <<= (Stmt >>= (Skip | Assign | While | If | Output | Block | Return | Var)))
		| (If <<= BExpr * (Then >>= Stmt) * (Else >>= Stmt))
		| (While <<= BExpr * (Do >>= Stmt))
		| (Assign <<= Ident * (Rhs >>= AExpr))
		| (Output <<= AExpr)	
		| (Block <<= Stmt++[1])
		| (Return <<= AExpr)
		;

	inline const wf::Wellformed eval_wf =
		(statements_wf - Top);

	inline const wf::Wellformed normalization_wf =
		(statements_wf - Var)
		| (Stmt <<= (Stmt >>= (Skip | Assign | While | If | Output | Block | Return)))
		| (AExpr <<= (Expr >>= (Atom | Add | Sub | Mul | FunCall)))
		| (Atom <<= (Expr >>= (Int | Ident | Input)))
		| (Add <<= (Lhs >>= Atom) * (Rhs >>= Atom))
		| (Sub <<= (Lhs >>= Atom) * (Rhs >>= Atom))
		| (Mul <<= (Lhs >>= Atom) * (Rhs >>= Atom))
		| (LT <<= (Lhs >>= Atom) * (Rhs >>= Atom))
		| (Equals <<= (Lhs >>= Atom) * (Rhs >>= Atom))
		| (Output <<= Atom)
		| (Return <<= Atom)
		| (Arg <<= Atom)
		;
}
