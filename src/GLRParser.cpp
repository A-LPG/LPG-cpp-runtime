#include "lpg2/GLRParser.h"

#include <deque>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "lpg2/BacktrackingParser.h"
#include "lpg2/Exception.h"
#include "lpg2/IAst.h"
#include "lpg2/ILexStream.h"
#include "lpg2/IToken.h"
#include "lpg2/Monitor.h"
#include "lpg2/Object.h"
#include "lpg2/ParseTable.h"
#include "lpg2/ParseTableProxy.h"
#include "lpg2/RuleAction.h"
#include "lpg2/TokenStream.h"

namespace
{

static Object nullResultSentinel;
static Object* const NULL_RESULT = &nullResultSentinel;

struct AcceptCandidate
{
    Object* ast;
    int grammarSymbol;

    AcceptCandidate(Object* ast, int grammarSymbol)
        : ast(ast), grammarSymbol(grammarSymbol)
    {
    }
};

struct Config
{
    std::vector<int> stateStack;
    std::vector<int> symbolStack;
    std::vector<Object*> parseStack;
    std::vector<int> locationStack;
    int stateStackTop = -1;
    int currentAction = 0;
    int curtok = 0;
    int lastToken = 0;
    int currentKind = 0;

    Config copy() const
    {
        return *this;
    }
};

struct ConfigKey
{
    const Config* config;
    int hash;

    explicit ConfigKey(const Config* config)
        : config(config)
    {
        int h = 31 * config->curtok + config->currentKind;
        h = 31 * h + config->lastToken;
        h = 31 * h + config->currentAction;
        for (int i = 0; i <= config->stateStackTop; i++)
        {
            h = 31 * h + config->stateStack[i];
            h = 31 * h + config->locationStack[i];
            h = 31 * h + config->symbolStack[i];
        }
        hash = h;
    }

    bool operator==(const ConfigKey& other) const
    {
        const Config* a = config;
        const Config* b = other.config;
        if (a->curtok != b->curtok || a->currentKind != b->currentKind
            || a->lastToken != b->lastToken
            || a->currentAction != b->currentAction
            || a->stateStackTop != b->stateStackTop)
            return false;
        for (int i = 0; i <= a->stateStackTop; i++)
        {
            if (a->stateStack[i] != b->stateStack[i]
                || a->locationStack[i] != b->locationStack[i]
                || a->symbolStack[i] != b->symbolStack[i])
                return false;
        }
        return true;
    }
};

struct ConfigKeyHash
{
    size_t operator()(const ConfigKey& key) const
    {
        return static_cast<size_t>(key.hash);
    }
};

struct ReductionKey
{
    int rule;
    int lastToken;
    std::vector<int> locations;
    std::vector<int> grammarSymbols;
    std::vector<Object*> semanticValues;
    int hash;

    ReductionKey(int rule, int lastToken, int rhs, int frameTop,
                 const std::vector<int>& locationStack,
                 const std::vector<int>& symbolStack,
                 const std::vector<Object*>& parseStack)
        : rule(rule), lastToken(lastToken)
    {
        locations.resize(rhs);
        grammarSymbols.resize(rhs);
        semanticValues.resize(rhs);
        int h = 31 * rule + lastToken;
        for (int i = 0; i < rhs; i++)
        {
            int index = frameTop + i;
            locations[i] = locationStack[index];
            grammarSymbols[i] = symbolStack[index];
            semanticValues[i] = parseStack[index];
            h = 31 * h + locations[i];
            h = 31 * h + grammarSymbols[i];
            h = 31 * h + static_cast<int>(
                reinterpret_cast<uintptr_t>(semanticValues[i]));
        }
        hash = h;
    }

    bool operator==(const ReductionKey& other) const
    {
        if (rule != other.rule || lastToken != other.lastToken
            || locations.size() != other.locations.size())
            return false;
        for (size_t i = 0; i < locations.size(); i++)
        {
            if (locations[i] != other.locations[i]
                || grammarSymbols[i] != other.grammarSymbols[i]
                || semanticValues[i] != other.semanticValues[i])
                return false;
        }
        return true;
    }
};

struct ReductionKeyHash
{
    size_t operator()(const ReductionKey& key) const
    {
        return static_cast<size_t>(key.hash);
    }
};

struct ForestKey
{
    int grammarSymbol;
    ILexStream* lexStream;
    int leftToken;
    int rightToken;

    ForestKey(int grammarSymbol, IAst* ast)
        : grammarSymbol(grammarSymbol)
    {
        IToken* left = ast->getLeftIToken();
        IToken* right = ast->getRightIToken();
        lexStream = left == nullptr ? nullptr : left->getILexStream();
        leftToken = left == nullptr ? -1 : left->getTokenIndex();
        rightToken = right == nullptr ? -1 : right->getTokenIndex();
    }

    bool isPackable() const
    {
        return leftToken >= 0 && rightToken >= 0;
    }

    bool operator==(const ForestKey& other) const
    {
        return grammarSymbol == other.grammarSymbol
            && lexStream == other.lexStream
            && leftToken == other.leftToken
            && rightToken == other.rightToken;
    }
};

struct ForestKeyHash
{
    size_t operator()(const ForestKey& key) const
    {
        int h = 31 * key.grammarSymbol
            + static_cast<int>(reinterpret_cast<uintptr_t>(key.lexStream));
        h = 31 * h + key.leftToken;
        return static_cast<size_t>(31 * h + key.rightToken);
    }
};

static void ensureCapacity(Config& c, int need, int stackIncrement)
{
    int len = static_cast<int>(c.stateStack.size());
    if (need < len)
        return;
    int neu = need + 8;
    if (neu < len + stackIncrement)
        neu = len + stackIncrement;
    if (static_cast<int>(c.stateStack.size()) < neu)
    {
        c.stateStack.resize(neu);
        c.symbolStack.resize(neu);
        c.parseStack.resize(neu);
        c.locationStack.resize(neu);
    }
}

static bool sameSpan(Object* a, Object* b)
{
    IAst* ia = dynamic_cast<IAst*>(a);
    IAst* ib = dynamic_cast<IAst*>(b);
    if (ia == nullptr || ib == nullptr)
        return false;
    IToken* la = ia->getLeftIToken();
    IToken* ra = ia->getRightIToken();
    IToken* lb = ib->getLeftIToken();
    IToken* rb = ib->getRightIToken();
    if (la == nullptr || ra == nullptr || lb == nullptr || rb == nullptr)
        return false;
    return la->getILexStream() == lb->getILexStream()
        && ra->getILexStream() == rb->getILexStream()
        && la->getTokenIndex() == lb->getTokenIndex()
        && ra->getTokenIndex() == rb->getTokenIndex();
}

static bool appendNextAst(Object* root, Object* alt, bool commit);

static bool canAppendNextAst(Object* root, Object* alt)
{
    return appendNextAst(root, alt, false);
}

static bool appendNextAst(Object* root, Object* alt)
{
    return appendNextAst(root, alt, true);
}

static bool appendNextAst(Object* root, Object* alt, bool commit)
{
    IAst* cur = dynamic_cast<IAst*>(root);
    IAst* neu = dynamic_cast<IAst*>(alt);
    if (cur == nullptr || neu == nullptr)
        return false;
    if (cur == neu)
        return true;

    std::unordered_set<IAst*> seen;
    IAst* tail = nullptr;
    for (IAst* p = cur; p != nullptr; p = p->getNextAst())
    {
        if (!seen.insert(p).second)
            return false;
        tail = p;
    }

    std::unordered_set<IAst*> incoming;
    for (IAst* p = neu; p != nullptr; )
    {
        if (!incoming.insert(p).second)
            return false;
        if (seen.find(p) != seen.end())
        {
            p = p->getNextAst();
            continue;
        }
        for (IAst* q = p->getNextAst(); q != nullptr; q = q->getNextAst())
        {
            if (!incoming.insert(q).second)
                return false;
            if (seen.find(q) != seen.end())
                return false;
        }
        if (commit)
        {
            tail->setNextAst(p);
            for (IAst* q = p; q != nullptr; q = q->getNextAst())
            {
                seen.insert(q);
                tail = q;
            }
        }
        return true;
    }
    return true;
}

static void packAccept(std::vector<AcceptCandidate>& accepts, const AcceptCandidate& cand)
{
    Object* ast = cand.ast;
    int grammarSymbol = cand.grammarSymbol;
    if (ast == NULL_RESULT)
    {
        for (size_t i = 0; i < accepts.size(); i++)
        {
            if (accepts[i].ast == NULL_RESULT)
                return;
        }
        accepts.push_back(cand);
        return;
    }
    if (ast == nullptr)
        return;
    for (size_t i = 0; i < accepts.size(); i++)
    {
        AcceptCandidate& existing = accepts[i];
        Object* a = existing.ast;
        if (a == NULL_RESULT)
            continue;
        if (existing.grammarSymbol == grammarSymbol
            && sameSpan(a, ast)
            && appendNextAst(a, ast))
            return;
    }
    accepts.push_back(cand);
}

static bool canPackParseStacks(const Config& existing, const Config& incoming)
{
    if (existing.stateStackTop != incoming.stateStackTop)
        return false;
    for (int i = 0; i <= existing.stateStackTop; i++)
    {
        Object* a = existing.parseStack[i];
        Object* b = incoming.parseStack[i];
        if (a == b)
            continue;
        if (dynamic_cast<IAst*>(a) == nullptr || dynamic_cast<IAst*>(b) == nullptr)
            return false;
        if (!sameSpan(a, b))
            return false;
        if (!canAppendNextAst(a, b))
            return false;
    }
    return true;
}

static Object* packSym(Object* a, Object* b)
{
    if (a == nullptr)
        return b;
    if (b == nullptr || a == b)
        return a;
    if (!appendNextAst(a, b))
        throw std::runtime_error("overlapping GLR semantic forests");
    return a;
}

static void packParseStacks(Config& existing, const Config& incoming)
{
    for (int i = 0; i <= existing.stateStackTop; i++)
    {
        Object* a = existing.parseStack[i];
        Object* b = incoming.parseStack[i];
        if (a == b || a == nullptr || b == nullptr)
            continue;
        if (!canAppendNextAst(a, b))
            throw std::runtime_error("overlapping GLR semantic forests");
    }
    for (int i = 0; i <= existing.stateStackTop; i++)
        existing.parseStack[i] = packSym(existing.parseStack[i], incoming.parseStack[i]);
}

static void applyReduceClosure(
    Config& fork,
    int rule,
    std::vector<Config>& work,
    GLRParser* parser,
    ParseTableProxy* prs,
    RuleAction* ra,
    int NUM_RULES,
    int NT_OFFSET,
    std::unordered_map<ReductionKey, IAst*, ReductionKeyHash>& familyCache,
    std::unordered_map<ForestKey, IAst*, ForestKeyHash>& forestCache)
{
    int action = rule;
    do
    {
        int rhs = prs->rhs(action);
        if (fork.stateStackTop - (rhs - 1) < 0)
            throw std::runtime_error("GLR reduce stack underflow");

        fork.stateStackTop -= (rhs - 1);
        ReductionKey reductionKey(
            action, fork.lastToken, rhs, fork.stateStackTop,
            fork.locationStack, fork.symbolStack, fork.parseStack);
        parser->currentAction = action;
        parser->lastToken = fork.lastToken;
        parser->frameTop = fork.stateStackTop;
        parser->frameLocation = fork.locationStack.data();
        parser->frameParse = fork.parseStack.data();

        parser->taking_actions = true;
        ra->ruleAction(action);
        parser->taking_actions = false;

        int lhs = prs->lhs(action);
        int lhsSymbol = NT_OFFSET + lhs;
        Object* result = fork.parseStack[fork.stateStackTop];
        if (dynamic_cast<IAst*>(result) != nullptr)
        {
            auto famIt = familyCache.find(reductionKey);
            IAst* canonical = nullptr;
            if (famIt == familyCache.end())
            {
                IAst* ast = dynamic_cast<IAst*>(result);
                ForestKey forestKey(lhsSymbol, ast);
                if (forestKey.isPackable())
                {
                    auto forestIt = forestCache.find(forestKey);
                    if (forestIt != forestCache.end())
                        canonical = forestIt->second;
                }
                if (canonical == nullptr)
                {
                    canonical = ast;
                    if (forestKey.isPackable())
                        forestCache[forestKey] = canonical;
                }
                else if (canonical != ast && !appendNextAst(canonical, ast))
                    throw std::runtime_error("cannot merge GLR production family");
                familyCache[reductionKey] = canonical;
            }
            else
                canonical = famIt->second;
            fork.parseStack[fork.stateStackTop] = canonical;
        }
        fork.symbolStack[fork.stateStackTop] = lhsSymbol;
        action = prs->ntAction(fork.stateStack[fork.stateStackTop], lhs);
    } while (action <= NUM_RULES);

    fork.currentAction = action;
    work.push_back(fork);
}

static void applyConcreteAction(
    Config& fork,
    int cand,
    std::vector<Config>& work,
    std::vector<Config>& out,
    std::vector<AcceptCandidate>& accepts,
    GLRParser* parser,
    TokenStream* tokStream,
    ParseTableProxy* prs,
    RuleAction* ra,
    int NUM_RULES,
    int NT_OFFSET,
    int ACCEPT_ACTION,
    int ERROR_ACTION,
    int parseStackRoot,
    std::unordered_map<ReductionKey, IAst*, ReductionKeyHash>& familyCache,
    std::unordered_map<ForestKey, IAst*, ForestKeyHash>& forestCache)
{
    if (cand <= NUM_RULES)
    {
        fork.stateStackTop--;
        applyReduceClosure(fork, cand, work, parser, prs, ra, NUM_RULES, NT_OFFSET,
                           familyCache, forestCache);
    }
    else if (cand > ERROR_ACTION)
    {
        fork.symbolStack[fork.stateStackTop] = fork.currentKind;
        fork.lastToken = fork.curtok;
        fork.curtok = tokStream->getNext(fork.curtok);
        fork.currentKind = tokStream->getKind(fork.curtok);
        applyReduceClosure(fork, cand - ERROR_ACTION, work, parser, prs, ra, NUM_RULES, NT_OFFSET,
                           familyCache, forestCache);
    }
    else if (cand < ACCEPT_ACTION)
    {
        fork.symbolStack[fork.stateStackTop] = fork.currentKind;
        fork.lastToken = fork.curtok;
        fork.curtok = tokStream->getNext(fork.curtok);
        fork.currentKind = tokStream->getKind(fork.curtok);
        fork.currentAction = cand;
        out.push_back(fork);
    }
    else if (cand == ACCEPT_ACTION)
    {
        Object* root = nullptr;
        int rootSymbol = 0;
        if (!fork.parseStack.empty()
            && parseStackRoot < static_cast<int>(fork.parseStack.size()))
            root = fork.parseStack[parseStackRoot];
        if (parseStackRoot <= fork.stateStackTop)
            rootSymbol = fork.symbolStack[parseStackRoot];
        accepts.emplace_back(root == nullptr ? NULL_RESULT : root, rootSymbol);
    }
}

static void stepConfig(
    Config& cfg,
    std::vector<Config>& out,
    std::vector<AcceptCandidate>& accepts,
    GLRParser* parser,
    TokenStream* tokStream,
    ParseTableProxy* prs,
    RuleAction* ra,
    int NUM_RULES,
    int NT_OFFSET,
    int LA_STATE_OFFSET,
    int ACCEPT_ACTION,
    int ERROR_ACTION,
    int parseStackRoot,
    int stackIncrement,
    std::unordered_map<ReductionKey, IAst*, ReductionKeyHash>& familyCache,
    std::unordered_map<ForestKey, IAst*, ForestKeyHash>& forestCache)
{
    std::vector<Config> work;
    work.push_back(cfg.copy());
    int guard = prs->NUM_STATES * 4 + 8;

    while (!work.empty())
    {
        if (--guard < 0)
            throw std::runtime_error(
                "cyclic/ε-loop grammar not supported by GLR v1");

        Config c = work.back();
        work.pop_back();
        ensureCapacity(c, c.stateStackTop + 2, stackIncrement);
        c.stateStack[++c.stateStackTop] = c.currentAction;
        c.locationStack[c.stateStackTop] = c.curtok;
        c.symbolStack[c.stateStackTop] = 0;
        if (c.stateStackTop != parseStackRoot)
            c.parseStack[c.stateStackTop] = nullptr;

        int act = parser->tAction(c.currentAction, c.currentKind, c.curtok);
        std::vector<int> candidates;
        if (act > ACCEPT_ACTION && act < ERROR_ACTION)
            parser->expandConflict(act, candidates);
        else
            candidates.push_back(act);

        for (size_t ci = 0; ci < candidates.size(); ci++)
        {
            int cand = candidates[ci];
            Config fork = (candidates.size() == 1) ? c : c.copy();
            applyConcreteAction(fork, cand, work, out, accepts, parser, tokStream, prs, ra,
                                NUM_RULES, NT_OFFSET, ACCEPT_ACTION, ERROR_ACTION,
                                parseStackRoot, familyCache, forestCache);
        }
    }
}

} // namespace

int GLRParser::lookahead(int act, int token)
{
    act = prs->lookAhead(act - LA_STATE_OFFSET, tokStream->getKind(token));
    return (act > LA_STATE_OFFSET
                ? lookahead(act, tokStream->getNext(token))
                : act);
}

int GLRParser::tAction(int state, int sym, int curtok)
{
    int act = prs->tAction(state, sym);
    return (act > LA_STATE_OFFSET
                ? lookahead(act, tokStream->getNext(curtok))
                : act);
}

void GLRParser::expandConflict(int act, std::vector<int>& out)
{
    for (int i = act; ; i++)
    {
        int cand = prs->baseAction(i);
        if (cand == 0)
            break;
        out.push_back(cand);
    }
}

int GLRParser::getCurrentRule()
{
    if (taking_actions)
        return currentAction;
    throw UnavailableParserInformationException();
}

int GLRParser::getToken(int i)
{
    if (taking_actions)
        return frameLocation[frameTop + (i - 1)];
    return Stacks::getToken(i);
}

Object* GLRParser::getSym(int i)
{
    if (taking_actions)
        return frameParse[frameTop + (i - 1)];
    return Stacks::getSym(i);
}

void GLRParser::setSym1(Object* ast)
{
    if (taking_actions)
        frameParse[frameTop] = ast;
    else
        Stacks::setSym1(ast);
}

int GLRParser::getFirstToken()
{
    if (taking_actions)
        return getToken(1);
    throw UnavailableParserInformationException();
}

int GLRParser::getFirstToken(int i)
{
    if (taking_actions)
        return getToken(i);
    throw UnavailableParserInformationException();
}

int GLRParser::getLastToken()
{
    if (taking_actions)
        return lastToken;
    throw UnavailableParserInformationException();
}

int GLRParser::getLastToken(int i)
{
    if (taking_actions)
        return (i >= prs->rhs(currentAction)
                    ? lastToken
                    : tokStream->getPrevious(getToken(i + 1)));
    throw UnavailableParserInformationException();
}

void GLRParser::setMonitor(Monitor* monitor)
{
    this->monitor = monitor;
}

void GLRParser::reset()
{
    taking_actions = false;
}

void GLRParser::reset(Monitor* monitor, TokenStream* tokStream)
{
    this->monitor = monitor;
    this->tokStream = tokStream;
    reset();
}

void GLRParser::reset(TokenStream* tokStream)
{
    reset(nullptr, tokStream);
}

void GLRParser::reset(Monitor* monitor, TokenStream* tokStream, ParseTable* prs, RuleAction* ra)
{
    reset(monitor, tokStream);

    delete this->prs;
    this->prsTable = prs;
    this->prs = new ParseTableProxy(prs);
    this->ra = ra;

    START_STATE = prs->getStartState();
    NUM_RULES = prs->getNumRules();
    NT_OFFSET = prs->getNtOffset();
    LA_STATE_OFFSET = prs->getLaStateOffset();
    ACCEPT_ACTION = prs->getAcceptAction();
    ERROR_ACTION = prs->getErrorAction();

    if (!prs->isValidForParser())
        throw BadParseSymFileException();
    if (!prs->isGLR())
        throw NotGLRParseTableException();
}

GLRParser::~GLRParser()
{
    delete prs;
}

GLRParser::GLRParser()
{
}

GLRParser::GLRParser(TokenStream* tokStream, ParseTable* prs, RuleAction* ra)
{
    reset(nullptr, tokStream, prs, ra);
}

GLRParser::GLRParser(Monitor* monitor, TokenStream* tokStream, ParseTable* prs, RuleAction* ra)
{
    reset(monitor, tokStream, prs, ra);
}

Object* GLRParser::parse()
{
    return parseEntry(0);
}

Object* GLRParser::parse(int max_error_count)
{
    return parseEntry(0, max_error_count);
}

Object* GLRParser::parseEntry(int marker_kind)
{
    return parseEntryNoRepair(marker_kind);
}

Object* GLRParser::parseEntry(int marker_kind, int max_error_count)
{
    try
    {
        return parseEntryNoRepair(marker_kind);
    }
    catch (BadParseException&)
    {
        if (max_error_count <= 0)
            throw;
        BacktrackingParser bt(monitor, tokStream, prsTable, ra);
        ra->setRecoverParser(&bt);
        try
        {
            Object* result = bt.fuzzyParseEntry(marker_kind, max_error_count);
            ra->setRecoverParser(nullptr);
            return result;
        }
        catch (...)
        {
            ra->setRecoverParser(nullptr);
            throw;
        }
    }
}

Object* GLRParser::parseEntryNoRepair(int marker_kind)
{
    tokStream->reset();
    std::unordered_map<ReductionKey, IAst*, ReductionKeyHash> familyCache;
    std::unordered_map<ForestKey, IAst*, ForestKeyHash> forestCache;

    int firstTok = tokStream->getToken();
    int prev = tokStream->getPrevious(firstTok);
    int startTok = marker_kind == 0 ? firstTok : prev;
    int startKind = marker_kind == 0 ? tokStream->getKind(firstTok) : marker_kind;
    parseStackRoot = marker_kind == 0 ? 0 : 1;

    Config start;
    start.stateStackTop = -1;
    start.currentAction = START_STATE;
    start.curtok = startTok;
    start.lastToken = prev;
    start.currentKind = startKind;
    ensureCapacity(start, 16, STACK_INCREMENT);

    std::deque<Config> live;
    live.push_back(start);

    std::vector<AcceptCandidate> accepts;
    int errorTok = startTok;
    int outerGuard = prs->NUM_STATES * 64 + tokStream->getStreamLength() * 8 + 256;

    while (!live.empty())
    {
        if (monitor != nullptr && monitor->isCancelled())
            return nullptr;
        if (--outerGuard < 0)
            throw std::runtime_error(
                "cyclic/ε-loop grammar not supported by GLR v1");

        std::deque<Config> next;
        std::unordered_map<ConfigKey, std::vector<Config*>, ConfigKeyHash> packed;

        for (Config& cfg : live)
        {
            if (cfg.curtok > errorTok)
                errorTok = cfg.curtok;

            std::vector<Config> stepResults;
            std::vector<AcceptCandidate> stepAccepts;
            stepConfig(cfg, stepResults, stepAccepts, this, tokStream, prs, ra,
                       NUM_RULES, NT_OFFSET, LA_STATE_OFFSET, ACCEPT_ACTION, ERROR_ACTION,
                       parseStackRoot, STACK_INCREMENT, familyCache, forestCache);

            for (const AcceptCandidate& a : stepAccepts)
                packAccept(accepts, a);

            for (Config& r : stepResults)
            {
                ConfigKey k(&r);
                auto bucketIt = packed.find(k);
                if (bucketIt == packed.end())
                {
                    next.push_back(r);
                    Config* rp = &next.back();
                    std::vector<Config*> bucket;
                    bucket.push_back(rp);
                    packed.emplace(ConfigKey(rp), bucket);
                }
                else
                {
                    bool merged = false;
                    for (Config* existing : bucketIt->second)
                    {
                        if (canPackParseStacks(*existing, r))
                        {
                            packParseStacks(*existing, r);
                            merged = true;
                            break;
                        }
                    }
                    if (!merged)
                    {
                        next.push_back(r);
                        bucketIt->second.push_back(&next.back());
                    }
                }
            }
        }

        if (!accepts.empty() && next.empty())
            break;

        live = std::move(next);
        if (live.empty() && accepts.empty())
            throw BadParseException(errorTok);
    }

    if (accepts.empty())
        throw BadParseException(errorTok);

    Object* root = accepts[0].ast;
    int rootSymbol = accepts[0].grammarSymbol;
    for (size_t i = 1; i < accepts.size(); i++)
    {
        AcceptCandidate& other = accepts[i];
        if (other.grammarSymbol != rootSymbol)
            throw std::runtime_error("GLR accepted distinct start symbols");
        if (!appendNextAst(root, other.ast))
            throw std::runtime_error("overlapping GLR accept forests");
    }
    return root == NULL_RESULT ? nullptr : root;
}
