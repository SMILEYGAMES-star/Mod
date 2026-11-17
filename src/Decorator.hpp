#pragma once
#include "Analyzer.hpp"

enum class StyleType { Neon, Organic, Shadow };

struct StyleSet { int edge, outer, inner, slope, spike, hang; };

inline StyleSet styleFor(StyleType t) {
    switch (t) {
        case StyleType::Neon:    return {467, 468, 469, 470, 471, 0};
        case StyleType::Organic: return {  1,   1,   1,   1,   1, 0};
        case StyleType::Shadow:  return { 40,  40,  40,  40,  40, 0};
    }
    return {};
}

struct Placement { int id; CCPoint pos; float rot; bool flipX, flipY; };

inline void sidePlacement(int side, CCPoint c, float& rot, CCPoint& pos) {
    const float h = 15.f;
    switch (side) {
        case UP:    rot = 0;   pos = {c.x, c.y + h}; break;
        case RIGHT: rot = 90;  pos = {c.x + h, c.y}; break;
        case DOWN:  rot = 180; pos = {c.x, c.y - h}; break;
        case LEFT:  rot = 270; pos = {c.x - h, c.y}; break;
    }
}

inline std::vector<Placement> planDecoration(Grid const& g, StyleType type) {
    auto S = styleFor(type);
    std::vector<Placement> out;
    const int dx[4] = {0, 1, 0, -1}, dy[4] = {1, 0, -1, 0};
    const int sides[4] = {UP, RIGHT, DOWN, LEFT};

    for (auto const& [key, cell] : g.cells) {
        auto [gx, gy] = key;
        CCPoint c = {gx * 30.f, gy * 30.f};

        if (cell.kind == Kind::Block) {
            for (int i = 0; i < 4; i++) {
                int nx = gx + dx[i], ny = gy + dy[i];
                if (g.solid(nx, ny)) continue;
                int a = (i + 1) % 4, b = (i + 3) % 4;
                bool openA = !g.solid(gx + dx[a], gy + dy[a]);
                bool openB = !g.solid(gx + dx[b], gy + dy[b]);
                float rot; CCPoint pos; sidePlacement(sides[i], c, rot, pos);

                auto* nb = g.get(nx, ny);
                if (nb && nb->kind == Kind::Slope) { out.push_back({S.slope, pos, rot, false, false}); continue; }

                if (openA && openB)      out.push_back({S.outer, pos, rot, false, false});
                else if (openA)          out.push_back({S.outer, pos, rot, false, false});
                else if (openB)          out.push_back({S.outer, pos, rot, true,  false});
                else                     out.push_back({S.edge,  pos, rot, false, false});

                if (!openA && !openB) {
                    for (int s : {a, b}) {
                        int ddx = dx[i] + dx[s], ddy = dy[i] + dy[s];
                        if (!g.solid(gx + ddx, gy + ddy) && !g.has(gx + ddx, gy + ddy))
                            out.push_back({S.inner, {c.x + ddx * 15.f, c.y + ddy * 15.f}, rot, s == b, false});
                    }
                }
                if (type == StyleType::Organic && S.hang && sides[i] == DOWN)
                    out.push_back({S.hang, {pos.x, pos.y - 15.f}, 0, false, false});
            }
        }
        else if (cell.kind == Kind::Slope) {
            int hyp = (UP|RIGHT|DOWN|LEFT) & ~cell.solidSides;
            float rot = 0;
            if (hyp == (UP|RIGHT))        rot = 315;
            else if (hyp == (DOWN|RIGHT)) rot = 45;
            else if (hyp == (DOWN|LEFT))  rot = 135;
            else if (hyp == (UP|LEFT))    rot = 225;
            float ox = ((hyp & RIGHT) ? 1 : -1) * 8.f, oy = ((hyp & UP) ? 1 : -1) * 8.f;
            out.push_back({S.slope, {c.x + ox, c.y + oy}, rot, false, false});
        }
        else {
            int baseSide = rotateSides(cell.dangerSide, 180);
            float rot; CCPoint pos; sidePlacement(baseSide, c, rot, pos);
            out.push_back({S.spike, pos, rot, false, false});
        }
    }
    return out;
}
