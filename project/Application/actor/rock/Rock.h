#pragma once
#include "math/Vector3.h"

// フィールドに落ちている「3D岩」
// 当たり判定は XZ 平面の円
class Rock {
public:
  Vector3 position_; // ワールド座標
  float radius_;     // XZ 上の半径
  bool isAlive_;     // 有効かどうか（拾われたら false）

    // ==== スポーン演出用 ====
    bool  isSpawning_ = false;
    float spawnTimer_ = 0.0f;
    float spawnDuration_ = 0.3f;

    float spawnStartY_ = 0.0f; // 地面より少し下の Y
    float spawnEndY_ = 0.0f; // 地面の Y（最終）

public:
    Rock();
    Rock(const Vector3& pos, float radius);
  bool isAttached_ = false;
  Vector3 localDir_{0.0f, 0.0f, 1.0f};
  float distanceFromPlayer_ = 0.0f;


    void Kill();

    // スポーンなどの時間変化
    void Update(float deltaTime);

private:
    // 簡易 BackOut イージング
    static float EaseOutBack(float t);
};
