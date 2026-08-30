#pragma once

#include <map>
#include <string>
#include "Resource/Model/Data/NodeAnimation.h"

struct Animation {
    float duration; // アニメーション全体の尺(単位は秒)
    // NodeAnimationの集合。Node名でひけるようにしておく
    std::map<std::string, NodeAnimation> nodeAnimations;
};