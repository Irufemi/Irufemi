#pragma once

#include "math/Vector2.h"

//地面との当たり判定
struct CollisionResult {
    bool isColliding;    // 衝突したか
    Vector2 closest;     // 最近接点
    Vector2 normal;      // 法線（点 → 最近接点方向）
    float penetration;   // めり込み距離（プレイヤー半径とかで使う）
};