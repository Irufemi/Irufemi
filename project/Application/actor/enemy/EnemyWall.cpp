#define NONMINMAX
#include "EnemyWall.h"
#include "manager/DebugUI.h"
#include <algorithm>
#include <cmath>
#include <random>

// 乱数ユーティリティ（ファイルローカル）
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
  models_.clear();

  walls_.reserve(maxWallCount_);
  models_.reserve(maxWallCount_);
}

void EnemyWallManager::Update(float deltaTime) {
  for (auto &wall : walls_) {
    if (!wall.active) {
      continue;
    }

    wall.lifeTime -= deltaTime;
    if (wall.lifeTime <= 0.0f) {
      wall.active = false;
    }
  }

  int activeCount = 0;
  for (const auto &w : walls_) {
    if (w.active) {
      ++activeCount;
    }
  }

#if defined USE_IMGUI
  ImGui::Text("Active Walls: %d", activeCount);
#endif
}

void EnemyWallManager::Draw() {
  // ★ 壁数とモデル数は常に同じ長さのはず
  const int count = static_cast<int>(walls_.size());
  for (int i = 0; i < count; ++i) {
    const EnemyWall &wall = walls_[i];
    if (!wall.active) {
      continue;
    }

    ObjClass *model = models_[i].get();
    if (!model) {
      continue;
    }

    model->SetPosition(wall.position);
    model->Update();
    model->Draw();
  }
}

// 壁生成アクション：敵の位置を基準に「同時に3つ」生成
void EnemyWallManager::SpawnWalls(const Vector3 &enemyPos) {
  int currentCount = 0;
  for (const auto &w : walls_) {
    if (w.active) {
      ++currentCount;
    }
  }

  const int kSpawnNum = 3;
  if (currentCount >= maxWallCount_) {
    return;
  }

  for (int i = 0; i < kSpawnNum; ++i) {
    if (currentCount >= maxWallCount_) {
      break;
    }

    const int kMaxTry = 20;
    bool spawned = false;

    for (int t = 0; t < kMaxTry; ++t) {
      Vector3 pos = GenerateRandomPosition(enemyPos);
      if (!CanPlaceWallAt(pos, enemyPos)) {
        continue;
      }

      // ★ 空きスロットのインデックスを探す
      int freeIndex = -1;
      for (int idx = 0; idx < static_cast<int>(walls_.size()); ++idx) {
        if (!walls_[idx].active) {
          freeIndex = idx;
          break;
        }
      }

      // ★ なければ新規スロットを作る
      if (freeIndex == -1) {
        if (static_cast<int>(walls_.size()) >= maxWallCount_) {
          // もう増やせない
          spawned = false;
          break;
        }

        walls_.emplace_back();
        models_.emplace_back(std::make_unique<ObjClass>());
        models_.back()->Initialize(camera_, "cube.obj");

        freeIndex = static_cast<int>(walls_.size()) - 1;
      }

      EnemyWall &w = walls_[freeIndex];
      w.position = pos;
      w.halfSizeX = 1.5f;
      w.halfSizeZ = 0.5f;
      w.lifeTime = wallLifeTime_;
      w.active = true;

      spawned = true;
      ++currentCount;
      break;
    }

    if (!spawned) {
      // この1個は諦める
    }
  }
}

// 円（center, radius）とアクティブな壁との衝突チェック
int EnemyWallManager::CheckCollisionCircle(const Vector3 &center,
                                           float radius) const {
  for (size_t i = 0; i < walls_.size(); ++i) {
    const auto &w = walls_[i];
    if (!w.active) {
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

  // 仕様に応じてここで w.active = false; してもいい
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

// 内部：この位置に壁を置けるかどうか
bool EnemyWallManager::CanPlaceWallAt(const Vector3 &pos,
                                      const Vector3 &enemyPos) const {
  float margin = 1.0f;
  float r = std::sqrt(1.5f * 1.5f + 0.5f * 0.5f);
  float distFromCenter = DistanceXZ(pos, stageCenter_);
  if (distFromCenter + r > stageRadius_ - margin) {
    return false;
  }

  float avoidEnemyRadius = 5.0f;
  if (DistanceXZ(pos, enemyPos) < avoidEnemyRadius) {
    return false;
  }

  for (const auto &w : walls_) {
    if (!w.active) {
      continue;
    }

    float dx = pos.x - w.position.x;
    float dz = pos.z - w.position.z;
    float minDistX = 1.5f + w.halfSizeX + 0.5f;
    float minDistZ = 0.5f + w.halfSizeZ + 0.5f;

    if (std::abs(dx) < minDistX && std::abs(dz) < minDistZ) {
      return false;
    }
  }

  return true;
}

// 内部：リング領域（中心付近と外周を避ける）でランダム位置を返す
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
