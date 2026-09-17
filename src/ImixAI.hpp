#pragma once

namespace cocos2d { class CCLabelTTF; }
class PlayLayer;

namespace ImixAI {
    void update(PlayLayer* layer, float dt);
    bool enabled();
    cocos2d::CCLabelTTF* createOverlay();
    void reset();
}
