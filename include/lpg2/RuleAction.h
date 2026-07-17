#pragma once
#include <vector>
#include "AstPoolHolder.h"
#include "ProstheticAst.h"

struct RuleAction
{
	pool_holder _automatic_ast_pool;
	virtual ~RuleAction() = default;
	virtual   void ruleAction(int ruleNumber) = 0;

	//
	// Parsers generated with automatic_ast and %Recover symbols override this
	// to return an array of prosthetic-AST factories indexed by the value of
	// ParseTable::getProsthesisIndex(kind). The default (no recover symbols)
	// returns nullptr, in which case the backtracking parser keeps its
	// historical behavior of throwing a BadParseException on a replayed
	// nonterminal token.
	//
	virtual std::vector<ProstheticAst>* getProstheticAst() { return nullptr; }
};
