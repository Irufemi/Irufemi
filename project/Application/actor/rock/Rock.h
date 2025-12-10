#pragma once
#include "contents/Quaternion.h"
#include "math/Vector3.h"

// フィールドに落ちている「3D岩」
// 当たり判定は XZ 平面の円
class Rock {
public:
  Vector3 position_; // ワールド座標
  float radius_;     // XZ 上の半径
  bool isAlive_;     // 有効かどうか（拾われたら false）

  Vector3 velocity_{0.0f, 0.0f, 0.0f};
  bool isDropped_ = false;

  // ==== スポーン演出用 ====
  bool isSpawning_ = false;
  float spawnTimer_ = 0.0f;
  float spawnDuration_ = 0.3f;

  float spawnStartY_ = 0.0f; // 地面より少し下の Y
  float spawnEndY_ = 0.0f;   // 地面の Y（最終）

  Vector3 rotate_{};
  Quaternion localRotation_{0.0f, 0.0f, 0.0f, 1.0f};

  // ==== 場外に出たときの縮小演出用 ====
  bool  isShrinking_ = false; // 縮小中か？
  float shrinkTimer_ = 0.0f;  // 経過時間
  float shrinkDuration_ = 1.0f;  // 何秒かけて消えるか
  float shrinkStartRadius_ = 0.0f; // 縮小開始時の半径

public:
  Rock();
  Rock(const Vector3 &pos, float radius);
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
