#pragma once

#include <vector>

#include "Stacks.h"

class ParseTableProxy;
struct IAst;
struct Monitor;
struct ParseTable;
struct RuleAction;
struct TokenStream;

struct GLRParser : public Stacks
{
    Monitor* monitor = nullptr;
    int START_STATE = 0;
    int NUM_RULES = 0;
    int NT_OFFSET = 0;
    int LA_STATE_OFFSET = 0;
    int ACCEPT_ACTION = 0;
    int ERROR_ACTION = 0;

    TokenStream* tokStream = nullptr;
    ParseTableProxy* prs = nullptr;
    RuleAction* ra = nullptr;

    bool taking_actions = false;
    int currentAction = 0;
    int lastToken = 0;
    int parseStackRoot = 0;
    int frameTop = 0;
    int* frameLocation = nullptr;
    Object** frameParse = nullptr;

    int lookahead(int act, int token);
    int tAction(int state, int sym, int curtok);
    void expandConflict(int act, std::vector<int>& out);

    int getCurrentRule();
    int getToken(int i);
    Object* getSym(int i);
    void setSym1(Object* ast);
    int getFirstToken();
    int getFirstToken(int i);
    int getLastToken();
    int getLastToken(int i);

    void setMonitor(Monitor* monitor);
    void reset();
    void reset(Monitor* monitor, TokenStream* tokStream);
    void reset(TokenStream* tokStream);
    void reset(Monitor* monitor, TokenStream* tokStream, ParseTable* prs, RuleAction* ra);
    void reset(TokenStream* tokStream, ParseTable* prs, RuleAction* ra)
    {
        reset(nullptr, tokStream, prs, ra);
    }

    ~GLRParser();
    GLRParser();
    GLRParser(TokenStream* tokStream, ParseTable* prs, RuleAction* ra);
    GLRParser(Monitor* monitor, TokenStream* tokStream, ParseTable* prs, RuleAction* ra);

    Object* parse();
    Object* parseEntry(int marker_kind);
};
