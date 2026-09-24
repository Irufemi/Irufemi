#pragma once

#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"

namespace Irufemi {
struct Line {
    Vector3 origin; //!< 始点
    Vector3 diff;   //!< 終点への差分ベクトル
};

struct Ray {
    Vector3 origin; //!< 始点
    Vector3 diff;   //!< 終点への差分ベクトル
};

struct Segment2D {
    Vector2 origin; ///< 始点座標
    Vector2 end;    ///< 終点座標
};

struct Segment {
    Vector3 origin; //!< 始点
    Vector3 diff;   //!< 終点への差分ベクトル
};

} // namespace Irufemi
