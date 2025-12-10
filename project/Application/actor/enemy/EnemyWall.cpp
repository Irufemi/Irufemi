#define NONMINMAX
#include "EnemyWall.h"
#include "Application/camera/Camera.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {

std::mt19937 &GetRng() {
  static std::random_device rd;
  static std::mt19937 mt(rd());
  return mt;
}

float RandomRange(float minValue, float maxValue) {
  std::uniform_real_distribution<float> dist(minValue, maxValue);
  return dist(GetRng());
}

} // namespace

static float DistanceXZ(const Vector3 &a, const Vector3 &b) {
  float dx = a.x - b.x;
  float dz = a.z - b.z;
  return std::sqrt(dx * dx + dz * dz);
}

EnemyWallManager::EnemyWallManager() {}

void EnemyWallManager::Initialize(Camera *camera, const Vector3 &stageCenter,
                                  float stageRadius) {
  camera_ = camera;
  stageCenter_ = stageCenter;
  stageRadius_ = stageRadius;

  walls_.clear();
  walls_.resize(maxWallCount_);
  for (int i = 0; i < maxWallCount_; ++i) {
    walls_[i] = EnemyWall{};
    walls_[i].active = false;
  }

  // 描画用 Region を初期化（モデルは1回だけロード）
  wallRegion_.Initialize(camera_, "wall.obj");
  warningRegion_.Initialize(camera_, "warning.obj");

  wallRegion_.AddInstance(Transform{});
}

void EnemyWallManager::Update(float deltaTime) {
  for (auto &wall : walls_) {
    if (!wall.active) {
      continue;
    }

    if (!wall.hasLanded) {
      if (wall.warningTime > 0.0f) {
        wall.warningTime -= deltaTime;
        if (wall.warningTime <= 0.0f) {
          wall.warningTime = 0.0f;
          wall.fallTimer = wall.fallDuration;
          wall.position.y = wall.groundY + wall.fallStartHeight;
        }
      } else if (wall.fallTimer > 0.0f) {
        wall.fallTimer -= deltaTime;
        float t = 1.0f - (wall.fallTimer / wall.fallDuration);
        t = (std::max)(0.0f, (std::min)(t, 1.0f));
        wall.position.y = wall.groundY + wall.fallStartHeight * (1.0f - t);

        if (wall.fallTimer <= 0.0f) {
          wall.position.y = wall.groundY;
          wall.hasLanded = true;
        }
      }
    }

    wall.lifeTime -= deltaTime;
    if (wall.lifeTime <= 0.0f) {
      wall.active = false;
    }
  }
}

void EnemyWallManager::Draw() {
  // インスタンスリストを構築
  wallRegion_.ClearInstances();
  warningRegion_.ClearInstances();

  const int count = static_cast<int>(walls_.size());
  for (int i = 0; i < count; ++i) {
    const EnemyWall &wall = walls_[i];
    if (!wall.active) {
      continue;
    }

    Vector3 wallRotate{0.0f, wall.yawRadians, 0.0f};

    // 影（warning.obj）
    if (!wall.hasLanded) {
      Transform t;
      Vector3 shadowPos = wall.position;
      shadowPos.y = wall.groundY + 0.05f;

      const float kMinScale = warningScaleMin_;
      const float kMaxScale = warningScaleMax_;

      float scaleFactor = kMinScale;
      if (wall.warningTime > 0.0f) {
        scaleFactor = kMinScale;
      } else if (wall.fallDuration > 0.0f) {
        float t01 = 1.0f - (wall.fallTimer / wall.fallDuration);
        t01 = (std::max)(0.0f, (std::min)(t01, 1.0f));
        scaleFactor = kMinScale + (kMaxScale - kMinScale) * t01;
      }

      t.translate = shadowPos;
      t.rotate = {0.0f, wallRotate.y, 0.0f};
      t.scale = {wall.halfSizeX * scaleFactor, 1.0f,
                 wall.halfSizeZ * scaleFactor};

      warningRegion_.AddInstance(t);
    }

    // 本体（wall.obj）
    if (wall.warningTime <= 0.0f) {
      Transform t;
      t.translate = wall.position;
      t.rotate = {0.0f, wallRotate.y, 0.0f};

      const float baseHalfX = 0.5f;
      const float baseHalfZ = 0.5f;
      float scaleX = wall.halfSizeX / baseHalfX;
      float scaleZ = wall.halfSizeZ / baseHalfZ;
      t.scale = {scaleX, 1.0f, scaleZ};

      wallRegion_.AddInstance(t);
    }
  }

  // 先に影、その後本体を描画
  warningRegion_.Draw();
  wallRegion_.Draw();
}

void EnemyWallManager::SpawnWalls(const Vector3 &enemyPos,
                                  const Vector3 &playerPos) {
  int currentCount = 0;
  for (const auto &w : walls_) {
    if (w.active) {
      currentCount++;
    }
  }

  const int kSpawnNum = 3;
  if (currentCount >= maxWallCount_) {
    return;
  }

  const float candidateHalfX = 0.5f;
  const float candidateHalfZ = 0.5f;

  for (int i = 0; i < kSpawnNum; ++i) {
    if (currentCount >= maxWallCount_) {
      break;
    }

    const int kMaxTry = 20;
    bool spawned = false;

    for (int t = 0; t < kMaxTry; ++t) {
      Vector3 pos = GenerateRandomPosition(enemyPos);

      if (!CanPlaceWallAt(pos, enemyPos, playerPos, candidateHalfX,
                          candidateHalfZ)) {
        continue;
      }

      int freeIndex = -1;
      for (int idx = 0; idx < static_cast<int>(walls_.size()); ++idx) {
        if (!walls_[idx].active) {
          freeIndex = idx;
          break;
        }
      }
      if (freeIndex == -1) {
        spawned = false;
        break;
      }

      EnemyWall &w = walls_[freeIndex];
      w.position = pos;
      w.position.y = stageCenter_.y;
      w.halfSizeX = candidateHalfX;
      w.halfSizeZ = candidateHalfZ;

      w.groundY = stageCenter_.y;
      w.fallStartHeight = wallFallStartHeight_;
      w.fallDuration = wallFallDuration_;
      w.fallTimer = 0.0f;
      w.warningTime = wallWarningTime_;
      w.hasLanded = false;
      w.yawRadians = 0.0f;

      w.lifeTime = wallLifeTime_ + wallFallDuration_ + wallWarningTime_;
      w.active = true;

      spawned = true;
      ++currentCount;
      break;
    }
  }
}

void EnemyWallManager::SpawnWallLine3x1(const Vector3 &enemyPos,
                                        const Vector3 &playerPos) {
  int currentCount = 0;
  for (const auto &w : walls_) {
    if (w.active) {
      ++currentCount;
    }
  }

  const int kSpawnNum = 3;
  const int kMaxTry = 20;

  const float candidateHalfX = 1.5f;
  const float candidateHalfZ = 0.5f;

  if (currentCount >= maxWallCount_) {
    return;
  }

  for (int i = 0; i < kSpawnNum; ++i) {
    if (currentCount >= maxWallCount_) {
      break;
    }

    bool spawned = false;

    for (int t = 0; t < kMaxTry; ++t) {
      Vector3 pos = GenerateRandomPosition(enemyPos);

      if (!CanPlaceWallAt(pos, enemyPos, playerPos, candidateHalfX,
                          candidateHalfZ)) {
        continue;
      }

      int freeIndex = -1;
      for (int idx = 0; idx < static_cast<int>(walls_.size()); ++idx) {
        if (!walls_[idx].active) {
          freeIndex = idx;
          break;
        }
      }
      if (freeIndex == -1) {
        spawned = false;
        break;
      }

      EnemyWall &w = walls_[freeIndex];
      w.position = pos;
      w.position.y = stageCenter_.y;

      w.halfSizeX = candidateHalfX;
      w.halfSizeZ = candidateHalfZ;

      w.groundY = stageCenter_.y;
      w.fallStartHeight = wallFallStartHeight_;
      w.fallDuration = wallFallDuration_;
      w.fallTimer = 0.0f;
      w.warningTime = wallWarningTime_;
      w.hasLanded = false;
      w.yawRadians = RandomRange(0.0f, 6.28318530718f);

      w.lifeTime = wallLifeTime_ + wallFallDuration_ + wallWarningTime_;
      w.active = true;

      spawned = true;
      ++currentCount;
      break;
    }
  }
}

int EnemyWallManager::CheckCollisionCircle(const Vector3 &center,
                                           float radius) const {
  for (size_t i = 0; i < walls_.size(); ++i) {
    const auto &w = walls_[i];
    if (!w.active) {
      continue;
    }
    if (!w.hasLanded) {
      continue;
    }

    float dx = center.x - w.position.x;
    float dz = center.z - w.position.z;

    float clampedX = (std::max)(-w.halfSizeX, (std::min)(dx, w.halfSizeX));
    float clampedZ = (std::max)(-w.halfSizeZ, (std::min)(dz, w.halfSizeZ));

    float nearestX = w.position.x + clampedX;
    float nearestZ = w.position.z + clampedZ;

    float diffX = center.x - nearestX;
    float diffZ = center.z - nearestZ;

    float distSq = diffX * diffX + diffZ * diffZ;
    float rSq = radius * radius;

    if (distSq <= rSq) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void EnemyWallManager::OnPlayerHitWall(int wallIndex) {
  if (wallIndex < 0 || wallIndex >= static_cast<int>(walls_.size())) {
    return;
  }
  EnemyWall &w = walls_[wallIndex];
  if (!w.active) {
    return;
  }
  w.active = false;
}

void EnemyWallManager::OnEnemyHitWall(int wallIndex) {
  if (wallIndex < 0 || wallIndex >= static_cast<int>(walls_.size())) {
    return;
  }
  EnemyWall &w = walls_[wallIndex];
  if (!w.active) {
    return;
  }
  w.active = false;
}

bool EnemyWallManager::CanPlaceWallAt(const Vector3 &pos,
                                      const Vector3 &enemyPos,
                                      const Vector3 &playerPos,
                                      float candidateHalfX,
                                      float candidateHalfZ) const {
  float margin = 1.0f;

  float candidateRadius = std::sqrt(candidateHalfX * candidateHalfX +
                                    candidateHalfZ * candidateHalfZ);

  float distFromCenter = DistanceXZ(pos, stageCenter_);
  if (distFromCenter + candidateRadius > stageRadius_ - margin) {
    return false;
  }

  float avoidEnemyRadius = 5.0f + candidateRadius;
  if (DistanceXZ(pos, enemyPos) < avoidEnemyRadius) {
    return false;
  }

  float avoidPlayerRadius = 5.0f + candidateRadius;
  if (DistanceXZ(pos, playerPos) < avoidPlayerRadius) {
    return false;
  }

  for (const auto &w : walls_) {
    if (!w.active) {
      continue;
    }

    float dx = pos.x - w.position.x;
    float dz = pos.z - w.position.z;

    float minDistX = candidateHalfX + w.halfSizeX + 0.5f;
    float minDistZ = candidateHalfZ + w.halfSizeZ + 0.5f;

    if (std::abs(dx) < minDistX && std::abs(dz) < minDistZ) {
      return false;
    }
  }

  return true;
}

Vector3
EnemyWallManager::GenerateRandomPosition(const Vector3 &enemyPos) const {
  (void)enemyPos;

  float innerRadius = 5.0f;
  float outerRadius = stageRadius_ - 3.0f;

  float angle = RandomRange(0.0f, 6.28318530718f);
  float radius = RandomRange(innerRadius, outerRadius);

  Vector3 pos;
  pos.x = stageCenter_.x + std::cos(angle) * radius;
  pos.y = stageCenter_.y;
  pos.z = stageCenter_.z + std::sin(angle) * radius;
  return pos;
}
