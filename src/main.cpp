#include <Geode/modify/EditorUI.hpp>
#include "Decorator.hpp"

class $modify(AutoDecoUI, EditorUI) {
    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;
        auto menu = CCMenu::create();
        menu->setPosition({60.f, 250.f});
        menu->setLayout(ColumnLayout::create()->setAxisReverse(true));
        const char* names[3] = {"Neon", "Organic", "Shadow"};
        for (int i = 0; i < 3; i++) {
            auto spr = ButtonSprite::create(names[i], 60, true, "bigFont.fnt", "GJ_button_01.png", 30.f, .6f);
            auto btn = CCMenuItemExt::createSpriteExtra(spr, [this, i](CCObject*) { this->runDeco((StyleType)i); });
            menu->addChild(btn);
        }
        menu->updateLayout();
        this->addChild(menu, 100);
        return true;
    }

    void runDeco(StyleType type) {
        auto* lel = m_editorLayer;
        Grid g = buildGrid(lel->m_objects);
        auto plan = planDecoration(g, type);
        int n = 0;
        for (auto& p : plan) {
            if (!p.id) continue;
            auto* o = lel->createObject(p.id, p.pos, false);
            if (!o) continue;
            o->setRotation(p.rot);
            if (p.flipX) o->setFlipX(true);
            if (p.flipY) o->setFlipY(true);
            n++;
        }
        FLAlertLayer::create("AutoDeco", fmt::format("{} objetos de decoracao criados.", n), "OK")->show();
    }
};
