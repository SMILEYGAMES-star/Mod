#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <map>
#include <vector>
#include <cmath>
#include <string>

using namespace geode::prelude;

namespace ad {
    enum Kind { K_BLOCK, K_SLOPE, K_SPIKE, K_SMALL };
    enum Side { S_UP = 1, S_RIGHT = 2, S_DOWN = 4, S_LEFT = 8 };

    struct Cell { int kind = K_BLOCK; int solid = 0; int danger = 0; };
    typedef std::map<std::pair<int,int>, Cell> Grid;

    struct Place { int id; float x; float y; float rot; bool flip; };

    struct StyleSet { int edge; int outer; int inner; int slope; int spike; };

    StyleSet styleFor(int s) {
        StyleSet r;
        if (s == 0) { r.edge = 467; r.outer = 468; r.inner = 469; r.slope = 470; r.spike = 471; }
        else if (s == 1) { r.edge = 1; r.outer = 1; r.inner = 1; r.slope = 1; r.spike = 1; }
        else { r.edge = 40; r.outer = 40; r.inner = 40; r.slope = 40; r.spike = 40; }
        return r;
    }

    bool isBlock(int id) { return id == 1 || (id >= 2 && id <= 7) || id == 40; }
    bool isSlope(int id) { return id == 289 || id == 291 || id == 294 || id == 296; }
    bool isSpike(int id) { return id == 8 || id == 9 || id == 61 || id == 103 || id == 392; }
    bool isSmall(int id) { return id == 39 || id == 135 || id == 363 || id == 364 || id == 365; }

    int rotateSides(int mask, int deg) {
        int steps = (((deg % 360) + 360) % 360) / 90;
        int order[4] = {S_UP, S_RIGHT, S_DOWN, S_LEFT};
        int out = 0;
        for (int i = 0; i < 4; i++) {
            if (mask & order[i]) out |= order[(i + steps) % 4];
        }
        return out;
    }

    int flipSides(int mask, bool fx, bool fy) {
        int out = mask;
        if (fx) {
            bool r = (mask & S_RIGHT) != 0;
            bool l = (mask & S_LEFT) != 0;
            out &= ~(S_LEFT | S_RIGHT);
            if (r) out |= S_LEFT;
            if (l) out |= S_RIGHT;
        }
        if (fy) {
            bool u = (mask & S_UP) != 0;
            bool d = (mask & S_DOWN) != 0;
            out &= ~(S_UP | S_DOWN);
            if (u) out |= S_DOWN;
            if (d) out |= S_UP;
        }
        return out;
    }

    bool isSolid(Grid const& g, int x, int y) {
        auto it = g.find(std::make_pair(x, y));
        return it != g.end() && it->second.kind == K_BLOCK;
    }

    bool hasCell(Grid const& g, int x, int y) {
        return g.find(std::make_pair(x, y)) != g.end();
    }

    Grid buildGrid(CCArray* objects) {
        Grid g;
        for (auto* o : CCArrayExt<GameObject*>(objects)) {
            int id = o->m_objectID;
            CCPoint p = o->getPosition();
            int gx = (int)std::floor(p.x / 30.f + 0.5f);
            int gy = (int)std::floor(p.y / 30.f + 0.5f);
            int rot = (int)std::round(o->getRotation() / 90.f) * 90;
            bool fx = o->isFlipX();
            bool fy = o->isFlipY();

            Cell c;
            if (isBlock(id)) {
                c.kind = K_BLOCK;
                c.solid = S_UP | S_RIGHT | S_DOWN | S_LEFT;
            } else if (isSlope(id)) {
                c.kind = K_SLOPE;
                c.solid = rotateSides(flipSides(S_DOWN | S_LEFT, fx, fy), rot);
            } else if (isSpike(id) || isSmall(id)) {
                c.kind = isSpike(id) ? K_SPIKE : K_SMALL;
                c.danger = rotateSides(flipSides(S_UP, fx, fy), rot);
            } else {
                continue;
            }
            g[std::make_pair(gx, gy)] = c;
        }
        return g;
    }

    void sidePos(int side, float cx, float cy, float& rot, float& px, float& py) {
        const float h = 15.f;
        px = cx; py = cy; rot = 0;
        if (side == S_UP)         { rot = 0;   py = cy + h; }
        else if (side == S_RIGHT) { rot = 90;  px = cx + h; }
        else if (side == S_DOWN)  { rot = 180; py = cy - h; }
        else if (side == S_LEFT)  { rot = 270; px = cx - h; }
    }

    void add(std::vector<Place>& out, int id, float x, float y, float rot, bool flip) {
        Place p; p.id = id; p.x = x; p.y = y; p.rot = rot; p.flip = flip;
        out.push_back(p);
    }

    std::vector<Place> plan(Grid const& g, int style) {
        StyleSet S = styleFor(style);
        std::vector<Place> out;
        const int dx[4] = {0, 1, 0, -1};
        const int dy[4] = {1, 0, -1, 0};
        const int sides[4] = {S_UP, S_RIGHT, S_DOWN, S_LEFT};

        for (auto it = g.begin(); it != g.end(); ++it) {
            int gx = it->first.first;
            int gy = it->first.second;
            Cell const& cell = it->second;
            float cx = gx * 30.f;
            float cy = gy * 30.f;

            if (cell.kind == K_BLOCK) {
                for (int i = 0; i < 4; i++) {
                    int nx = gx + dx[i];
                    int ny = gy + dy[i];
                    if (isSolid(g, nx, ny)) continue;
                    int a = (i + 1) % 4;
                    int b = (i + 3) % 4;
                    bool openA = !isSolid(g, gx + dx[a], gy + dy[a]);
                    bool openB = !isSolid(g, gx + dx[b], gy + dy[b]);
                    float rot, px, py;
                    sidePos(sides[i], cx, cy, rot, px, py);

                    auto nb = g.find(std::make_pair(nx, ny));
                    if (nb != g.end() && nb->second.kind == K_SLOPE) {
                        add(out, S.slope, px, py, rot, false);
                        continue;
                    }

                    if (openA && openB)  add(out, S.outer, px, py, rot, false);
                    else if (openA)      add(out, S.outer, px, py, rot, false);
                    else if (openB)      add(out, S.outer, px, py, rot, true);
                    else                 add(out, S.edge,  px, py, rot, false);

                    if (!openA && !openB) {
                        int idx[2] = {a, b};
                        for (int k = 0; k < 2; k++) {
                            int s = idx[k];
                            int ddx = dx[i] + dx[s];
                            int ddy = dy[i] + dy[s];
                            if (!hasCell(g, gx + ddx, gy + ddy)) {
                                add(out, S.inner, cx + ddx * 15.f, cy + ddy * 15.f, rot, s == b);
                            }
                        }
                    }
                }
            }
            else if (cell.kind == K_SLOPE) {
                int all = S_UP | S_RIGHT | S_DOWN | S_LEFT;
                int hyp = all & ~cell.solid;
                float rot = 0;
                if (hyp == (S_UP | S_RIGHT)) rot = 315;
                else if (hyp == (S_DOWN | S_RIGHT)) rot = 45;
                else if (hyp == (S_DOWN | S_LEFT)) rot = 135;
                else if (hyp == (S_UP | S_LEFT)) rot = 225;
                float ox = (hyp & S_RIGHT) ? 8.f : -8.f;
                float oy = (hyp & S_UP) ? 8.f : -8.f;
                add(out, S.slope, cx + ox, cy + oy, rot, false);
            }
            else {
                int baseSide = rotateSides(cell.danger, 180);
                float rot, px, py;
                sidePos(baseSide, cx, cy, rot, px, py);
                add(out, S.spike, px, py, rot, false);
            }
        }
        return out;
    }
}

class $modify(AutoDecoUI, EditorUI) {
    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;

        auto menu = CCMenu::create();
        menu->setPosition({60.f, 250.f});
        menu->setLayout(ColumnLayout::create()->setAxisReverse(true));

        const char* names[3] = {"Neon", "Organic", "Shadow"};
        for (int i = 0; i < 3; i++) {
            auto spr = ButtonSprite::create(names[i], 60, true, "bigFont.fnt", "GJ_button_01.png", 30.f, .6f);
            auto btn = CCMenuItemExt::createSpriteExtra(spr, [this, i](CCObject*) {
                this->runDeco(i);
            });
            menu->addChild(btn);
        }
        menu->updateLayout();
        this->addChild(menu, 100);
        return true;
    }

    void runDeco(int style) {
        auto* lel = m_editorLayer;
        ad::Grid g = ad::buildGrid(lel->m_objects);
        std::vector<ad::Place> list = ad::plan(g, style);

        int n = 0;
        for (size_t i = 0; i < list.size(); i++) {
            ad::Place& p = list[i];
            if (p.id == 0) continue;
            auto* o = lel->createObject(p.id, CCPoint(p.x, p.y), false);
            if (!o) continue;
            o->setRotation(p.rot);
            if (p.flip) o->setFlipX(true);
            n++;
        }
        std::string msg = std::to_string(n) + " objetos criados.";
        FLAlertLayer::create("AutoDeco", msg.c_str(), "OK")->show();
    }
};
