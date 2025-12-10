#pragma once

#include "3D/Region.h"
#include "math/Vector3.h"
#include <vector>

class Camera;

struct EnemyWall {
  Vector3 position; // ワールド座標（中心）
  float halfSizeX;  // X方向半径
  float halfSizeZ;  // Z方向半径
  float lifeTime;   // 残り寿命（秒）
  bool active;      // 使用中フラグ

  // 落下アニメーション用
  float groundY;
  float fallStartHeight;
  float fallDuration;
  float fallTimer;
  float warningTime;
  bool hasLanded;

  float yawRadians;

  EnemyWall()
      : position(0.0f, 0.0f, 0.0f), halfSizeX(1.5f), halfSizeZ(0.5f),
        lifeTime(0.0f), active(false), groundY(0.0f), fallStartHeight(8.0f),
        fallDuration(0.5f), fallTimer(0.0f), warningTime(0.0f),
        hasLanded(false), yawRadians(0.0f) {}
};

class EnemyWallManager {
public:
  EnemyWallManager();

  void Initialize(Camera *camera, const Vector3 &stageCenter,
                  float stageRadius);

  void Update(float deltaTime);
  void Draw();

  void SpawnWalls(const Vector3 &enemyPos, const Vector3 &playerPos);
  void SpawnWallLine3x1(const Vector3 &enemyPos, const Vector3 &playerPos);

  const std::vector<EnemyWall> &GetWalls() const { return walls_; }

  int CheckCollisionCircle(const Vector3 &center, float radius) const;

  void OnPlayerHitWall(int wallIndex);
  void OnEnemyHitWall(int wallIndex);

  void SetWallLifeTime(float lifeTime) { wallLifeTime_ = lifeTime; }
  void SetMaxWallCount(int maxCount) { maxWallCount_ = maxCount; }

  void SetWallFallParameters(float startHeight, float duration,
                             float warningTime) {
    wallFallStartHeight_ = startHeight;
    wallFallDuration_ = duration;
    wallWarningTime_ = warningTime;
  }

private:
  Camera *camera_ = nullptr;

  // ロジック
  Vector3 stageCenter_;
  float stageRadius_{};
  std::vector<EnemyWall> walls_;

  float wallLifeTime_ = 8.0f;
  int maxWallCount_ = 9;

  float wallFallStartHeight_ = 12.0f;
  float wallFallDuration_ = 0.4f;
  float wallWarningTime_ = 0.6f;

  float warningScaleMin_ = 0.4f;
  float warningScaleMax_ = 1.6f;

  // 描画用：本体＆影をインスタンシング
  Region wallRegion_;
  Region warningRegion_;

  bool CanPlaceWallAt(const Vector3 &pos, const Vector3 &enemyPos,
                      const Vector3 &playerPos, float candidateHalfX,
                      float candidateHalfZ) const;

  Vector3 GenerateRandomPosition(const Vector3 &enemyPos) const;
};
