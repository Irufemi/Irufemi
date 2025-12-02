#pragma once
#include "math/Vector3.h"

// フィールドに落ちている「3D岩」
// 当たり判定は XZ 平面の円
class Rock {
public:
    Vector3 position_; // ワールド座標
    float   radius_;   // XZ 上の半径
    bool    isAlive_;  // 有効かどうか（拾われたら false）

    Rock()
        : position_{ 0.0f, 0.0f, 0.0f }
        , radius_(0.5f)
        , isAlive_(true) {
    }

    Rock(const Vector3& pos, float radius)
        : position_(pos)
        , radius_(radius)
        , isAlive_(true) {
    }

    void Kill() { isAlive_ = false; }

    void Update(float /*deltaTime*/) {
        // 必要になったら「動く岩」処理を書く
    }
};
