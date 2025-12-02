#include "Rock.h"
#include <cmath>

Rock::Rock()
    : position_{ 0.0f, 0.0f, 0.0f }
    , radius_(0.5f)
    , isAlive_(true) {

    // デフォルトはスポーン演出なし
    isSpawning_ = false;
    spawnTimer_ = 0.0f;
    spawnDuration_ = 0.3f;
    spawnStartY_ = position_.y;
    spawnEndY_ = position_.y;
}

Rock::Rock(const Vector3& pos, float radius)
    : position_(pos)
    , radius_(radius)
    , isAlive_(true) {

    const float kDepth = 1.0f;      // どれだけ下から生えるか

    spawnEndY_ = pos.y;          // 最終 Y
    spawnStartY_ = pos.y - kDepth; // 開始 Y（地面の下）
    position_.y = spawnStartY_;   // まずは下に置いておく

    spawnTimer_ = 0.0f;
    spawnDuration_ = 0.3f;         // 0.3秒くらいでニョキッ
    isSpawning_ = true;
}

void Rock::Kill() {
    isAlive_ = false;
}

float Rock::EaseOutBack(float t) {
    // 0〜1 の t に対する BackOut
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    t -= 1.0f;
    return 1.0f + c3 * t * t * t + c1 * t * t;
}

void Rock::Update(float deltaTime) {
    if (!isAlive_) {
        return;
    }

    // スポーン中アニメーション
    if (isSpawning_) {
        spawnTimer_ += deltaTime;

        float t = spawnTimer_ / spawnDuration_;
        if (t >= 1.0f) {
            t = 1.0f;
            isSpawning_ = false; // アニメ完了
        }

        float e = EaseOutBack(t); // 0→1 をちょいオーバーシュートで

        // 地面の下 → 地面まで補間
        position_.y = spawnStartY_ + (spawnEndY_ - spawnStartY_) * e;
    }

    // 将来「動く岩」にしたくなったら、この下に処理を足す
}
