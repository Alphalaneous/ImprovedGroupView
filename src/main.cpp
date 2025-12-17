#include <Geode/Geode.hpp>
#include "../api/GroupViewUpdateEvent.hpp"
#include "Geode/loader/Log.hpp"
#include <Geode/modify/SetGroupIDLayer.hpp>
#include <Geode/modify/CCScrollLayerExt.hpp>
#include <Geode/modify/SetupRandAdvTriggerPopup.hpp>
#include <Geode/modify/SetupSequenceTriggerPopup.hpp>
#include <Geode/modify/SetupSpawnPopup.hpp>
#define NAMED_EDITOR_GROUPS_USE_EVENTS_API
#include <spaghettdev.named-editor-groups/api/NIDManager.hpp>

using namespace geode::prelude;

std::string getNameForID(NID nid, short id) {
    std::string result;
    NIDManager::event::EventGetNameForID("spaghettdev.named-editor-groups/v2/get-name-for-id", &result, nid, id).post();
    log::debug("name: {} id: {} NID: {}", result, id, static_cast<int>(nid));
    return result;
}

class LimitedCCMenu : public CCMenu {

    public:

    geode::ScrollLayer* m_scrollLayer;

    static LimitedCCMenu* create() {
        auto ret = new LimitedCCMenu();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {

        if (m_scrollLayer) {
            CCSize scrollSize = m_scrollLayer->getScaledContentSize();
            CCPoint anchorPoint = m_scrollLayer->getAnchorPoint();

            float startPointX = m_scrollLayer->getPositionX() - scrollSize.width * anchorPoint.x;
            float startPointY = m_scrollLayer->getPositionY() - scrollSize.height * anchorPoint.y;

            CCRect rect = {startPointX, startPointY, scrollSize.width, scrollSize.height};

            if (rect.containsPoint(touch->getLocation())) {
                return CCMenu::ccTouchBegan(touch, event);
            }
        }
        else {
            return CCMenu::ccTouchBegan(touch, event);
        }
        return false;
    }
};

/*class $modify(SetupRandAdvTriggerPopup) {

    bool init(RandTriggerGameObject* p0, cocos2d::CCArray* p1) {
        if (!SetupRandAdvTriggerPopup::init(p0, p1)) return false;

        return true;
    }

};

class $modify(SetupSequenceTriggerPopup) {

    bool init(SequenceTriggerGameObject* p0) {
        if (!SetupSequenceTriggerPopup::init(p0)) return false;

        return true;
    }
    
};


class $modify(SetupSpawnPopup) {

    struct Fields {
        CCMenu* m_remapGroupsMenu;
    };

    bool init(EffectGameObject* p0, cocos2d::CCArray* p1) {

        auto fields = m_fields.self();

        fields->m_remapGroupsMenu = CCMenu::create();

        if (!SetupSpawnPopup::init(p0, p1)) return false;

        fields->m_remapGroupsMenu->setID("remap-groups-menu"_spr);
        fields->m_remapGroupsMenu->setContentSize({340, 85});

        RowLayout* layout = RowLayout::create();
        layout->setGrowCrossAxis(true);
        layout->setCrossAxisOverflow(false);

        fields->m_remapGroupsMenu->setLayout(layout);

        m_mainLayer->addChild(fields->m_remapGroupsMenu);

        handleTouchPriority(this);

        return true;
    }

    CCMenuItemSpriteExtra* createButton(int id, int remap, bool shared) {

        std::string texture = "GJ_button_04.png";
        if (shared) texture = "GJ_button_05.png";

        ButtonSprite* bspr = ButtonSprite::create(fmt::format("{}\n{}", id, remap).c_str(), 56, true, "goldFont.fnt", texture.c_str(), 30, 0.5);
        
        float width = 46;

        bspr->m_BGSprite->setContentSize({width, bspr->m_BGSprite->getContentHeight()});
        bspr->setContentSize(bspr->m_BGSprite->getScaledContentSize());
        bspr->m_BGSprite->setPosition(bspr->getContentSize()/2);

        CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(bspr, this, menu_selector(SetupSpawnPopup::onSelectRemap));
            
        //button->setTag(k);
        
        return button;
    }

    void setupGroupData(EffectGameObject* effectObj, std::map<std::pair<int, int>, bool>& groupMap, bool alwaysShared) {
        for (ChanceObject obj : static_cast<SpawnTriggerGameObject*>(effectObj)->m_remapObjects) {
            std::pair<int, int> data = {obj.m_oldGroupID, obj.m_chance};
            if (alwaysShared) {
                groupMap[data] = true;
            }
            else {
                if (groupMap.contains(data)) groupMap[data] = true;
                else groupMap[data] = false;
            }
        }
    }

    void updateRemapButtons(float p0) {
        SetupSpawnPopup::updateRemapButtons(p0);\
        
        auto fields = m_fields.self();

        fields->m_remapGroupsMenu->removeAllChildren();
        for (CCMenuItemSpriteExtra* btn : CCArrayExt<CCMenuItemSpriteExtra*>(m_remapButtons)) {
            btn->removeFromParentAndCleanup(false);
            //fields->m_remapGroupsMenu->addChild(btn);
        }

        std::map<std::pair<int, int>, bool> groups;

        if (m_gameObjects && m_gameObjects->count() > 1) {
            for (EffectGameObject* obj : CCArrayExt<EffectGameObject*>(m_gameObjects)) {
                setupGroupData(obj, groups, false);
            }
        }
        else if (m_gameObject) {
            setupGroupData(m_gameObject, groups, true);
        }

        for (auto [k, v] : groups) {
            log::info("group ID: {}, remap: {}, shared: {}", k.first, k.second, v);
            fields->m_remapGroupsMenu->addChild(createButton(k.first, k.second, v));
        }
        
        fields->m_remapGroupsMenu->updateLayout();
    }
};*/

class $modify(MySetGroupIDLayer, SetGroupIDLayer) {

    struct GroupData {
        std::vector<int> groups;
        std::vector<int> parentGroups;
        GameObject* object;
    };

    struct Fields {
        Ref<geode::ScrollLayer> m_scrollLayer;
        Ref<CCMenu> m_currentMenu;
        int m_lastRemoved = 0;
        float m_scrollPos = INT_MIN;
        std::unordered_map<std::string, short> m_namedIDs;

        EventListener<EventFilter<igv::GroupViewUpdateEvent>> m_apiListener;
    };

    static void onModify(auto& self) {
        (void) self.setHookPriorityBeforePost("SetGroupIDLayer::init", "spaghettdev.named-editor-groups");
    }

    bool init(GameObject* obj, cocos2d::CCArray* objs) {
        if (!SetGroupIDLayer::init(obj, objs)) {
            return false;
        }

        if (CCNode* node = m_mainLayer->getChildByID("groups-list-menu")) {

            CCMenu* replacementMenu = CCMenu::create();
            replacementMenu->setPosition(node->getPosition());
            replacementMenu->setContentSize(node->getContentSize());
            replacementMenu->setScaleX(node->getScaleX());
            replacementMenu->setScaleY(node->getScaleY());
            replacementMenu->setAnchorPoint(node->getAnchorPoint());
            replacementMenu->setID("z-layer-menu"_spr);

            m_mainLayer->addChild(replacementMenu);

            node->setVisible(false);
            
            if (CCNode* btn = node->getChildByID("z-layer-decrement-button")) {
                btn->removeFromParentAndCleanup(false);
                replacementMenu->addChild(btn);
            }
            if (CCNode* btn = node->getChildByID("z-layer-increment-button")) {
                btn->removeFromParentAndCleanup(false);
                replacementMenu->addChild(btn);
            }
        }

        if (CCNode* node = m_mainLayer->getChildByID("add-group-id-buttons-menu")) {
            
            if (CCMenuItemSpriteExtra* idBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(node->getChildByID("add-group-id-button"))) {
                idBtn->m_pfnSelector = menu_selector(MySetGroupIDLayer::onAddGroup2);
            }

            if (CCMenuItemSpriteExtra* parentBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(node->getChildByID("add-group-parent-button"))) {
                parentBtn->m_pfnSelector = menu_selector(MySetGroupIDLayer::onAddGroupParent2);
            }
        }

        regenerateGroupView();

        m_fields->m_apiListener.bind([this](igv::GroupViewUpdateEvent* event) {
            regenerateGroupView();

            return ListenerResult::Propagate;
        });

        return true;
    }

    void onRemoveFromGroup2(CCObject* obj) {
        m_fields->m_lastRemoved = obj->getTag();
        SetGroupIDLayer::onRemoveFromGroup(obj);
        regenerateGroupView();
    }

    void onAddGroup2(CCObject* obj) {
        SetGroupIDLayer::onAddGroup(obj);
        regenerateGroupView();
    }

    void onAddGroupParent2(CCObject* obj) {
        SetGroupIDLayer::onAddGroupParent(obj);
        regenerateGroupView();
    }

    void regenerateGroupView() {
        auto fields = m_fields.self();

        if (fields->m_scrollLayer) fields->m_scrollLayer->removeFromParent();

        std::vector<GroupData> groupData;
        std::map<int, int> allGroups;
        std::map<int, int> allParentGroups;

        if (!m_targetObjects || m_targetObjects->count() == 0) {
            if (m_targetObject) {
                groupData.push_back(parseObjGroups(m_targetObject));
            }
        }
        else {
            for (GameObject* obj : CCArrayExt<GameObject*>(m_targetObjects)) {
                groupData.push_back(parseObjGroups(obj));
            }
        }

        for (GroupData data : groupData) {
            for (int group : data.groups) {
                allGroups[group]++;
            }
            for (int group : data.parentGroups) {
                allParentGroups[group]++;
            }
        }

        allGroups.erase(0);
        allParentGroups.erase(0);

        if (allParentGroups.count(fields->m_lastRemoved)) {
            allParentGroups.erase(fields->m_lastRemoved);
        }
        else {
            allGroups.erase(fields->m_lastRemoved);
        }

        CCNode* menuContainer = CCNode::create();
        LimitedCCMenu* groupsMenu = LimitedCCMenu::create();

        RowLayout* layout = RowLayout::create();
        layout->setGap(12);
        layout->setAutoScale(false);
        layout->setGrowCrossAxis(true);
        layout->setCrossAxisOverflow(true);
        if (Mod::get()->getSettingValue<bool>("left-align")) {
            layout->setAxisAlignment(AxisAlignment::Start);
        }

        groupsMenu->setLayout(layout);

        fields->m_lastRemoved = 0;
        bool isNamed = Loader::get()->isModLoaded("spaghettdev.named-editor-groups");

        for (auto [k, v] : allGroups) {
            bool isParent = allParentGroups.count(k);
            bool isAlwaysPresent = v == groupData.size();

            std::string texture = "GJ_button_04.png";

            if (!isAlwaysPresent) texture = "GJ_button_05.png";
            if (isParent) texture = "GJ_button_03.png";

            std::string name = "";
            if (isNamed) {
                name = getNameForID(NID::GROUP, k);
            }

            ButtonSprite* bspr = ButtonSprite::create(fmt::format("{}", k).c_str(), 30, true, "goldFont.fnt", texture.c_str(), 20, 0.5);

            float width = 46;

            CCLabelBMFont* nameLabel = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
            nameLabel->setScale(0.5);

            if (!name.empty()) {
                bspr->m_label->setAnchorPoint({0.f, 0.5f});
                bspr->m_label->setPositionX(10);
                CCSize idLabelSize = bspr->m_label->getScaledContentSize();
                CCPoint idLabelPos = bspr->m_label->getPosition();
                nameLabel->setAnchorPoint({0.f, 0.5f});
                nameLabel->limitLabelWidth(70.f, .5, .1);
                width = nameLabel->getScaledContentWidth() + 25 + bspr->m_label->getScaledContentWidth();
                nameLabel->setPosition({bspr->m_label->getPositionX() + bspr->m_label->getScaledContentWidth() + 5.f, bspr->m_label->getPositionY()});
                bspr->addChild(nameLabel);

                auto background = CCSprite::create("square02b_001.png");
                background->setScaleX(idLabelSize.width / background->getScaledContentWidth() + .05f);
                background->setScaleY(idLabelSize.height / background->getScaledContentHeight() - .02f);
                background->setColor({0, 0, 0});
                background->setOpacity(100);
                background->setPosition({idLabelPos.x + idLabelSize.width/2, idLabelPos.y - 1.5f });
                bspr->addChild(background);
            }

            bspr->m_BGSprite->setContentSize({width, bspr->m_BGSprite->getContentHeight()});
            bspr->setContentSize(bspr->m_BGSprite->getScaledContentSize());
            bspr->m_BGSprite->setPosition(bspr->getContentSize()/2);

            CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(bspr, this, menu_selector(MySetGroupIDLayer::onRemoveFromGroup2));
            
            button->setTag(k);
            
            groupsMenu->addChild(button);
        }
        CCSize contentSize;

        if (groupsMenu->getChildrenCount() <= 10) {
            groupsMenu->setScale(1.f);
            contentSize = CCSize{278, 67};
        }
        else {
            groupsMenu->setScale(0.85f);
            contentSize = CCSize{395, 67};
        }

        float padding = 7.5;

        groupsMenu->setContentSize(contentSize);
        groupsMenu->setPosition({360.f / 2.f, padding});
        groupsMenu->setAnchorPoint({0.5, 0});
        groupsMenu->updateLayout();

        fields->m_currentMenu = groupsMenu;
        menuContainer->setContentSize({360, groupsMenu->getScaledContentSize().height + padding * 2});
        menuContainer->setAnchorPoint({0.5, 0});
        menuContainer->setPosition({360.f / 2.f, 0});
        menuContainer->addChild(groupsMenu);

        CCSize winSize = CCDirector::get()->getWinSize();

        fields->m_scrollLayer = ScrollLayer::create({360, menuContainer->getScaledContentSize().height});
        fields->m_scrollLayer->setContentSize({360, 68});
        fields->m_scrollLayer->setPosition({winSize.width/2, winSize.height/2 - 16.8f});
        fields->m_scrollLayer->ignoreAnchorPointForPosition(false);
        fields->m_scrollLayer->m_contentLayer->addChild(menuContainer);
        fields->m_scrollLayer->setID("groups-list-menu-scroll"_spr);
        groupsMenu->m_scrollLayer = fields->m_scrollLayer;

        m_mainLayer->addChild(fields->m_scrollLayer);
        if (fields->m_scrollPos == INT_MIN) {
            fields->m_scrollLayer->scrollToTop();
        }
        else {
            float minY = -(fields->m_scrollLayer->m_contentLayer->getContentSize().height - fields->m_scrollLayer->getContentSize().height);
            float pos = fields->m_scrollPos;

            if (fields->m_scrollPos < minY) pos = minY;
            if (fields->m_scrollPos > 0) pos = 0;

            fields->m_scrollLayer->m_contentLayer->setPositionY(pos);
        }

        if (menuContainer->getScaledContentHeight() <= 67) {
            fields->m_scrollLayer->m_disableMovement = true;
            fields->m_scrollLayer->enableScrollWheel(false);
        }

        m_mainLayer->removeChildByID("group-count"_spr);

        CCLabelBMFont* groupCountLabel = CCLabelBMFont::create(fmt::format("Groups: {}", allGroups.size()).c_str(), "chatFont.fnt");

        if (CCNode* zLayerLabel = m_mainLayer->getChildByID("z-layer-label")) {
            if (CCNode* groupsBG = m_mainLayer->getChildByID("groups-bg")) {
                CCPoint labelPos = zLayerLabel->getPosition();
                CCSize groupsBGSize = groupsBG->getContentSize();
                groupCountLabel->setPosition({labelPos.x - groupsBGSize.width/2, labelPos.y + 6});
            }
        }
        groupCountLabel->setID("group-count"_spr);
        groupCountLabel->setAnchorPoint({0, 0.5});
        groupCountLabel->setColor({0, 0, 0});
        groupCountLabel->setOpacity(200);
        groupCountLabel->setScale(0.5f);
        m_mainLayer->addChild(groupCountLabel);

        handleTouchPriority(this);

        #ifndef GEODE_IS_IOS
        queueInMainThread([this, fields] {
            if (auto delegate = typeinfo_cast<CCTouchDelegate*>(fields->m_scrollLayer.data())) {
                if (auto handler = CCTouchDispatcher::get()->findHandler(delegate)) {
                    CCTouchDispatcher::get()->setPriority(handler->getPriority() - 1, handler->getDelegate());
                }
            }
        });
        #endif
    }

    GroupData parseObjGroups(GameObject* obj) {

        LevelEditorLayer* lel = LevelEditorLayer::get();

        int uuid = obj->m_uniqueID;
        std::vector<int> parents;

        for (auto [k, v] : CCDictionaryExt<int, CCArray*>(lel->m_parentGroupIDs)) {
            if (k == uuid) {
                for (auto val : CCArrayExt<CCInteger*>(v)) {
                    parents.push_back(val->getValue());
                }
            }
        }

        std::vector<int> groups;

        if (obj->m_groups) {
            groups = std::vector<int>{obj->m_groups->begin(), obj->m_groups->end()};
        }

        return GroupData{groups, parents, obj};
    }

    static MySetGroupIDLayer* get() {

        CCScene* scene = CCDirector::get()->getRunningScene();
        return static_cast<MySetGroupIDLayer*>(scene->getChildByType<SetGroupIDLayer>(0));
    }

    void setButtonsEnabled(bool enabled) {
        if (CCMenu* menu = m_fields->m_currentMenu) {
            for (CCMenuItemSpriteExtra* btn : CCArrayExt<CCMenuItemSpriteExtra*>(menu->getChildren())) {
                btn->setEnabled(enabled);
                btn->stopAllActions();
                btn->setScale(1);
            }
        }
    }
};


// cheaty way to ensure groups don't accidentally get deleted, if you see this and wanna do better, please go for it :3

class $modify(CCScrollLayerExt) {

    void ccTouchMoved(cocos2d::CCTouch* p0, cocos2d::CCEvent* p1) {
        CCScrollLayerExt::ccTouchMoved(p0, p1);

        if (auto groupIDLayer = MySetGroupIDLayer::get()) {
            float dY = std::abs(p0->getStartLocation().y - p0->getLocation().y);
            if (dY > 3) {
                groupIDLayer->setButtonsEnabled(false);
            }
        }
    }

    void ccTouchEnded(cocos2d::CCTouch* p0, cocos2d::CCEvent* p1) {
        CCScrollLayerExt::ccTouchEnded(p0, p1);
        
        if (auto groupIDLayer = MySetGroupIDLayer::get()) {
            groupIDLayer->m_fields->m_scrollPos = m_contentLayer->getPositionY();
            groupIDLayer->setButtonsEnabled(true);
        }
    }
};