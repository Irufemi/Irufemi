#pragma once

#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include <cstdint>

struct AreaLight {
    //!< ライトの色
    Irufemi::Vector4 color;
    //!< ライトの位置
    Irufemi::Vector3 position;
    //!< 輝度
    float intensity;
    //!< ライトの向き
    Irufemi::Vector3 direction;
    //!< ライトの届く最大距離
    float range;
    //!< 矩形のサイズ(幅、高さ)
    Irufemi::Vector2 size;
    //!< 有効フラグ
    int32_t isActive;
private:
    float padding;
};