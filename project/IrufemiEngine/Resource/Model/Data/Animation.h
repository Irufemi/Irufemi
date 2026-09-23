#pragma once

#include <unordered_map>
#include <string>
#include "Resource/Model/Data/NodeAnimation.h"

namespace Irufemi {

/**
 * @struct Animation
 * @brief アニメーション全体を管理するデータ構造
 */
struct Animation {
    float duration = 0.0f; ///< アニメーション全体の尺(単位は秒)
    /// NodeAnimationの集合。Node名でひけるようにしておく
    std::unordered_map<std::string, NodeAnimation> nodeAnimations;
};

} // namespace Irufemi

using Irufemi::Animation;