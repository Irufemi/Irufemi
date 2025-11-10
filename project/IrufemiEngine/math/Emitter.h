#pragma once

#include "Transform.h"
#include <cstdint>

struct Emitter {
    //!< エミッタのトランスフォーム
    Transform transform;
    //!< 発生数
    uint32_t count; 
    //!< 発生頻度
    float frequency;
    //<! 頻度用時刻
    float frequencyTime;

};