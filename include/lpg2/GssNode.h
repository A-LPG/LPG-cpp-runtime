#pragma once

#include <vector>

struct SppfNode;
struct Object;
struct GssNode;

/** GSS edge to a predecessor, labeled with a recognized symbol / SPPF node. */
struct GssEdge
{
    GssNode* predecessor = nullptr;
    int symbol = 0;
    int location = 0;
    Object* semantic = nullptr;
    SppfNode* sppf = nullptr;

    GssEdge(GssNode* predecessor, int symbol, int location, Object* semantic, SppfNode* sppf)
        : predecessor(predecessor), symbol(symbol), location(location),
          semantic(semantic), sppf(sppf)
    {
    }
};

/** Graph-structured stack node: LR state at an input index. */
struct GssNode
{
    int state = 0;
    int index = 0;
    std::vector<GssEdge> edges;

    GssNode(int state, int index) : state(state), index(index) {}

    int getState() const { return state; }
    int getIndex() const { return index; }
    const std::vector<GssEdge>& getEdges() const { return edges; }
};
