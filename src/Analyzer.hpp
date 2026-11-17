#pragma once
#include <Geode/Geode.hpp>
#include <map>
#include <cmath>
using namespace geode::prelude;

enum class Kind { Block, Slope, Spike, SmallSpike };
enum Side { UP = 1, RIGHT = 2, DOWN = 4, LEFT = 8 };

struct Cell {
    Kind kind;
    int solidSides = 0;
    int dangerSide = 0;
    GameObject* obj = nullptr;
};

struct Grid {
    std::map<std::pair<int,int>, Cell> cells;
    bool has(int x, int y) const { return cells.count({x, y}) > 0; }
    bool solid(int x, int y) const {
        auto it = cells.find({x, y});
        return it != cells.end() && it->second.kind == Kind::Block;
    }
    const Cell* get(int x, int y) const {
        auto it = cells.find({x, y});
        return it == cells.end() ? nullptr : &it->second;
    }
};

inline bool isBlockID(int id)   { return id == 1 || (id >= 2 && id <= 7) || id == 40; }
inline bool isSlopeID(int id)   { return id == 289 || id == 291 || id == 294 || id == 296; }
inline bool isSpikeID(int id)   { return id == 8 || id == 9 || id == 61 || id == 103 || id == 392; }
inline bool isSmallSpikeID(int id) { return id == 39 || id == 135 || id == 363 || id == 364 || id == 365; }

inline int rotateSides(int mask, int deg) {
    int steps = ((deg % 360) + 360) % 360 / 90;
    int order[4] = {UP, RIGHT, DOWN, LEFT};
    int out = 0;
    for (int i = 0; i < 4; i++)
        if (mask & order[i]) out |= order[(i + steps) % 4];
    return out;
}

inline int flipSides(int mask, bool fx, bool fy) {
    int out = mask;
    if (fx) { bool r = mask & RIGHT, l = mask & LEFT; out &= ~(LEFT|RIGHT); if (r) out |= LEFT; if (l) out |= RIGHT; }
    if (fy) { bool u = mask & UP, d = mask & DOWN; out &= ~(UP|DOWN); if (u) out |= DOWN; if (d) out |= UP; }
    return out;
}

inline Grid buildGrid(CCArray* objects) {
    Grid g;
    for (auto* o : CCArrayExt<GameObject*>(objects)) {
        int id = o->m_objectID;
        auto p = o->getPosition();
        int gx = (int)std::floor(p.x / 30.f + 0.5f);
        int gy = (int)std::floor(p.y / 30.f + 0.5f);
        int rot = (int)std::round(o->getRotation() / 90.f) * 90;
        bool fx = o->isFlipX(), fy = o->isFlipY();

        Cell c; c.obj = o;
        if (isBlockID(id)) { c.kind = Kind::Block; c.solidSides = UP|RIGHT|DOWN|LEFT; }
        else if (isSlopeID(id)) {
            c.kind = Kind::Slope;
            c.solidSides = rotateSides(flipSides(DOWN|LEFT, fx, fy), rot);
        }
        else if (isSpikeID(id) || isSmallSpikeID(id)) {
            c.kind = isSpikeID(id) ? Kind::Spike : Kind::SmallSpike;
            c.dangerSide = rotateSides(flipSides(UP, fx, fy), rot);
        }
        else continue;
        g.cells[{gx, gy}] = c;
    }
    return g;
}
