#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "Stacks.h"

class ParseTableProxy;
struct IAst;
struct Monitor;
struct ParseTable;
struct RuleAction;
struct TokenStream;
struct SppfNode;
struct GssNode;

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
    ParseTable* prsTable = nullptr;
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

    /** Root SPPF symbol node from the last successful error-free parse, or null. */
    SppfNode* getSppfRoot() const { return sppfRoot; }

    /** Number of distinct SPPF symbol nodes created in the last parse. */
    int getSppfSymbolCount() const { return sppfSymbolCount; }

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
    Object* parse(int max_error_count);
    Object* parseEntry(int marker_kind);
    Object* parseEntry(int marker_kind, int max_error_count);

    // Owned forest / GSS storage for the current parse (public for driver helpers).
    std::vector<std::unique_ptr<SppfNode>> sppfStorage;
    std::vector<std::unique_ptr<GssNode>> gssStorage;
    std::unordered_map<long long, SppfNode*> sppfNodes;
    std::unordered_map<long long, GssNode*> gssNodes;
    SppfNode* sppfRoot = nullptr;
    int sppfSymbolCount = 0;

    SppfNode* sppfSymbol(int grammarSymbol, int leftExtent, int rightExtent);
    SppfNode* terminalSppf(int kind, int tok);
    void addPacked(SppfNode* symNode, int rule, const std::vector<SppfNode*>& children,
                   Object* semantic);
    GssNode* gssPush(GssNode* tip, int state, int index, int symbol, Object* semantic,
                     SppfNode* sppf);
    static GssNode* gssPop(GssNode* tip);
    GssNode* gssRelabel(GssNode* tip, int symbol, int location, Object* semantic, SppfNode* sppf);
    void clearForest();

private:
    Object* parseEntryNoRepair(int marker_kind);
};
