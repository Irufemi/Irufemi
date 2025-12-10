#include "Rock.h"
#include <cmath>

Rock::Rock() : position_{0.0f, 0.0f, 0.0f}, radius_(0.5f), isAlive_(true) {

  // デフォルトはスポーン演出なし
  isSpawning_ = false;
  spawnTimer_ = 0.0f;
  spawnDuration_ = 0.3f;
  spawnStartY_ = position_.y;
  spawnEndY_ = position_.y;

  // 縮小用
  isShrinking_ = false;
  shrinkTimer_ = 0.0f;
  shrinkDuration_ = 0.3f;
  shrinkStartRadius_ = radius_;
}

Rock::Rock(const Vector3 &pos, float radius)
    : position_(pos), radius_(radius), isAlive_(true) {

  const float kDepth = 1.0f; // どれだけ下から生えるか

  spawnEndY_ = pos.y;            // 最終 Y
  spawnStartY_ = pos.y - kDepth; // 開始 Y（地面の下）
  position_.y = spawnStartY_;    // まずは下に置いておく

  spawnTimer_ = 0.0f;
  spawnDuration_ = 0.3f; // 0.3秒くらいでニョキッ
  isSpawning_ = true;
}

void Rock::Kill() { isAlive_ = false; }

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

  if (isDropped_) {
    // 簡易重力
    const float kGravity = -30.0f;
    const float kAirDrag = 20.0f;

    // 縦方向の速度更新
    velocity_.y += kGravity * deltaTime;

    // 水平速度に減速をかける
    float vx = velocity_.x;
    float vz = velocity_.z;
    float speed = std::sqrt(vx * vx + vz * vz);

    if (speed > 0.0f) {
      float decel = kAirDrag * deltaTime;
      float newSpeed = speed - decel;
      if (newSpeed < 0.0f) {
        newSpeed = 0.0f;
      }

      float scale = (speed > 0.0f) ? (newSpeed / speed) : 0.0f;
      velocity_.x *= scale;
      velocity_.z *= scale;
    }

    // 位置更新
    position_.x += velocity_.x * deltaTime;
    position_.y += velocity_.y * deltaTime;
    position_.z += velocity_.z * deltaTime;

    // 地面に着いたら停止
    if (position_.y <= spawnEndY_) {
      position_.y = spawnEndY_;
      velocity_ = {0.0f, 0.0f, 0.0f};
      isDropped_ = false; // 以降は普通のフィールド岩
    }
  }

  // === 場外に出たあとの縮小アニメーション ===
  if (isShrinking_) {
      shrinkTimer_ += deltaTime;
      float t = shrinkTimer_ / shrinkDuration_;
      if (t >= 1.0f) {
          t = 1.0f;
      }

      // 半径を徐々に 0 にする
      radius_ = shrinkStartRadius_ * (1.0f - t);

      // 完全に縮小しきったら削除フラグ
      if (t >= 1.0f) {
          isAlive_ = false;
          isShrinking_ = false;
      }
  }

  // 将来「動く岩」にしたくなったら、この下に処理を足す
}
