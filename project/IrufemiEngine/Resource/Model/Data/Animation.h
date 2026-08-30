#pragma once

#include "Resource/Model/Data/NodeAnimation.h"
#include <map>
#include <string>

struct Animation {
    float duration; // アニメーション全体の尺(単位は秒)
    // NodeAnimationの集合。Node名でひけるようにしておく
    std::map<std::string, NodeAnimation> nodeAnimations;
};