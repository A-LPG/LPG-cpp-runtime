#pragma once
#include <functional>

struct IAst;
struct IToken;

//
// A ProstheticAst is a factory that synthesizes an AST node for a "recover"
// nonterminal. When the backtracking parser replays a nonterminal ErrorToken
// that was inserted by scope recovery, it asks the RuleAction for its
// prosthetic-AST factories and invokes one with the error token to build a
// placeholder (prosthetic) node in place of throwing a BadParseException.
//
using ProstheticAst = std::function<IAst*(IToken*)>;
