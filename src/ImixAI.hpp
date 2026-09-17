#pragma once

namespace cocos2d { class CCLayer; }
class PlayLayer;

namespace ImixAI {
    void update(PlayLayer* layer, float dt);
    void onDeath(PlayLayer* layer);
    bool enabled();
    cocos2d::CCLayer* createOverlay();
    void reset();
}
