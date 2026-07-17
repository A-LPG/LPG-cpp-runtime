#pragma once

#include "ParseTable.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

/**
 * Expected-terminals helper for editor completion (antlr4-c3 style).
 *
 * For parser state S, return sorted distinct terminal names where
 * tAction(S, sym) is not ERROR_ACTION. Names come from ParseTable::name
 * via terminalIndex(sym). Terminal symbol ids are 1 .. getNtOffset()-1
 * (0 is unused / error slot in LPG tables).
 */
inline std::vector<std::wstring> expectedTerminalNames(ParseTable* prs, int state)
{
    std::vector<std::wstring> out;
    if (!prs)
        return out;

    const int error_action = prs->getErrorAction();
    const int nt_offset = prs->getNtOffset();
    std::set<std::wstring> unique;
    for (int sym = 1; sym < nt_offset; ++sym) {
        const int act = prs->tAction(state, sym);
        if (act == error_action)
            continue;
        std::wstring n = prs->name(prs->terminalIndex(sym));
        if (n.empty())
            continue;
        unique.insert(n);
    }
    out.assign(unique.begin(), unique.end());
    return out;
}
