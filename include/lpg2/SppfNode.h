#pragma once

#include <vector>

struct Object;

/**
 * Shared packed parse forest node (GLR v2).
 * Symbol nodes are keyed by (grammarSymbol, leftExtent, rightExtent).
 */
struct SppfNode
{
    struct Packed
    {
        int rule = 0;
        std::vector<SppfNode*> children;
        Object* semantic = nullptr;

        Packed(int rule, std::vector<SppfNode*> children, Object* semantic)
            : rule(rule), children(std::move(children)), semantic(semantic)
        {
        }
    };

    int grammarSymbol = 0;
    int leftExtent = -1;
    int rightExtent = -1;
    std::vector<Packed> packs;
    Object* astForest = nullptr;

    SppfNode(int grammarSymbol, int leftExtent, int rightExtent)
        : grammarSymbol(grammarSymbol), leftExtent(leftExtent), rightExtent(rightExtent)
    {
    }

    int getGrammarSymbol() const { return grammarSymbol; }
    int getLeftExtent() const { return leftExtent; }
    int getRightExtent() const { return rightExtent; }
    const std::vector<Packed>& getPacks() const { return packs; }
    Object* getAstForest() const { return astForest; }
};
