#pragma once

#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"

// ボクセル化された個々のキューブの情報
struct Voxel {
    Vector3 position; // ワールド空間での中心位置
    Vector4 color;    // 元のテクスチャからサンプリングした色
    Vector3 normal;   // 元のモデルからサンプリングした法線
};