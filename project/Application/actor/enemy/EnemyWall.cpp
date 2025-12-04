#define NONMINMAX
#include "EnemyWall.h"
#include <algorithm>
#include <cmath>
#include <random>

// 乱数ユーティリティ（このファイル内専用）
namespace {

// 壁用の乱数生成器を返す（static で一度だけ初期化）
std::mt19937 &GetRng() {
  static std::random_device rd;
  static std::mt19937 mt(rd());
  return mt;
}

// min〜max の範囲でランダムな浮動小数を返す
float RandomRange(float minValue, float maxValue) {
  std::uniform_real_distribution<float> dist(minValue, maxValue);
  return dist(GetRng());
}
} // namespace

// 2点の XZ 平面上の距離を計算するヘルパー関数
static float DistanceXZ(const Vector3 &a, const Vector3 &b) {
  float dx = a.x - b.x;
  float dz = a.z - b.z;
  return std::sqrt(dx * dx + dz * dz);
}

EnemyWallManager::EnemyWallManager() {}

void EnemyWallManager::Initialize(Camera *camera, const Vector3 &stageCenter,
                                  float stageRadius) {
  // カメラとステージの中心・半径を記録しておく
  camera_ = camera;
  stageCenter_ = stageCenter;
  stageRadius_ = stageRadius;

  // 壁情報とモデル配列を初期化
  walls_.clear();
  models_.clear();

  // 最大個数分の容量を事前確保しておく
  walls_.reserve(maxWallCount_);
  models_.reserve(maxWallCount_);
}

void EnemyWallManager::Update(float deltaTime) {
  // すべての壁の寿命を更新し、寿命が尽きたら無効化する
  for (auto &wall : walls_) {
    if (!wall.active) {
      continue;
    }

    wall.lifeTime -= deltaTime;
    if (wall.lifeTime <= 0.0f) {
      wall.active = false;
    }
  }

  // 現在アクティブな壁の数を数える
  int activeCount = 0;
  for (const auto &w : walls_) {
    if (w.active) {
      ++activeCount;
    }
  }
}

void EnemyWallManager::Draw() {
  // 壁とモデルは同じ数並びで対応している前提
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

    // 壁の位置をモデルに反映して描画
    model->SetPosition(wall.position);
    model->Update();
    model->Draw();
  }
}

// 壁生成アクション：敵位置を中心に、同時に最大3つまで生成を試みる
void EnemyWallManager::SpawnWalls(const Vector3 &enemyPos) {
  // すでにアクティブな壁の数を数える
  int currentCount = 0;
  for (const auto &w : walls_) {
    if (w.active) {
      ++currentCount;
    }
  }

  const int kSpawnNum = 3;
  // 既に最大数に達しているなら何もしない
  if (currentCount >= maxWallCount_) {
    return;
  }

  // 3個分トライするが、途中で上限に達したら打ち切る
  for (int i = 0; i < kSpawnNum; ++i) {
    if (currentCount >= maxWallCount_) {
      break;
    }

    const int kMaxTry = 20;
    bool spawned = false;

    // この1個を置くために、最大 kMaxTry 回ランダム位置を試す
    for (int t = 0; t < kMaxTry; ++t) {
      // ステージ上のランダム位置を生成
      Vector3 pos = GenerateRandomPosition(enemyPos);

      // 配置して良い場所かどうかをチェック
      if (!CanPlaceWallAt(pos, enemyPos)) {
        continue;
      }

      // 空いているスロットを探す
      int freeIndex = -1;
      for (int idx = 0; idx < static_cast<int>(walls_.size()); ++idx) {
        if (!walls_[idx].active) {
          freeIndex = idx;
          break;
        }
      }

      // 空きが無ければ新しいスロットを作る
      if (freeIndex == -1) {
        if (static_cast<int>(walls_.size()) >= maxWallCount_) {
          // これ以上増やせないので諦める
          spawned = false;
          break;
        }

        walls_.emplace_back();
        models_.emplace_back(std::make_unique<ObjClass>());
        // 壁モデルを初期化（モデル名は環境に合わせて変更してもよい）
        models_.back()->Initialize(camera_, "cube.obj");

        freeIndex = static_cast<int>(walls_.size()) - 1;
      }

      // 壁情報を設定してアクティブ化
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
      // この1個は良い場所が見つからなかったという扱い（何もしない）
    }
  }
}

// 円（center, radius）とアクティブ壁との衝突をチェックする
int EnemyWallManager::CheckCollisionCircle(const Vector3 &center,
                                           float radius) const {
  // すべての壁に対して、円同士の交差（AABB 近似）をチェック
  for (size_t i = 0; i < walls_.size(); ++i) {
    const auto &w = walls_[i];
    if (!w.active) {
      continue;
    }

    // プレイヤー中心から壁中心へのベクトル
    float dx = center.x - w.position.x;
    float dz = center.z - w.position.z;

    // 壁の長方形に対して、dx/dz をクランプして最近接点を求める
    float clampedX = (std::max)(-w.halfSizeX, (std::min)(dx, w.halfSizeX));
    float clampedZ = (std::max)(-w.halfSizeZ, (std::min)(dz, w.halfSizeZ));

    float nearestX = w.position.x + clampedX;
    float nearestZ = w.position.z + clampedZ;

    // 最近接点と円の中心との距離を計算
    float diffX = center.x - nearestX;
    float diffZ = center.z - nearestZ;

    float distSq = diffX * diffX + diffZ * diffZ;
    float rSq = radius * radius;

    // 距離の二乗が半径の二乗以下なら衝突
    if (distSq <= rSq) {
      return static_cast<int>(i);
    }
  }

  // どの壁とも当たっていない
  return -1;
}

void EnemyWallManager::OnPlayerHitWall(int wallIndex) {
  // インデックスが範囲外なら無視
  if (wallIndex < 0 || wallIndex >= static_cast<int>(walls_.size())) {
    return;
  }

  EnemyWall &w = walls_[wallIndex];
  if (!w.active) {
    return;
  }

  // プレイヤー衝突時は壁を消す
  w.active = false;
}

void EnemyWallManager::OnEnemyHitWall(int wallIndex) {
  // インデックスが範囲外なら無視
  if (wallIndex < 0 || wallIndex >= static_cast<int>(walls_.size())) {
    return;
  }

  EnemyWall &w = walls_[wallIndex];
  if (!w.active) {
    return;
  }

  // 敵が壁に当たったら壁を消す仕様
  w.active = false;
}

// 内部関数：その位置に壁を置いてよいかチェックする
bool EnemyWallManager::CanPlaceWallAt(const Vector3 &pos,
                                      const Vector3 &enemyPos) const {
  // ステージ外周から少し内側の範囲だけを使う
  float margin = 1.0f;
  float r = std::sqrt(1.5f * 1.5f + 0.5f * 0.5f);
  float distFromCenter = DistanceXZ(pos, stageCenter_);
  if (distFromCenter + r > stageRadius_ - margin) {
    return false;
  }

  // 敵の近くには配置しないようにする
  float avoidEnemyRadius = 5.0f;
  if (DistanceXZ(pos, enemyPos) < avoidEnemyRadius) {
    return false;
  }

  // 既存の壁と近すぎないかチェックする
  for (const auto &w : walls_) {
    if (!w.active) {
      continue;
    }

    float dx = pos.x - w.position.x;
    float dz = pos.z - w.position.z;
    float minDistX = 1.5f + w.halfSizeX + 0.5f;
    float minDistZ = 0.5f + w.halfSizeZ + 0.5f;

    // XZ の両方向で十分に離れていない場合は重なっているとみなす
    if (std::abs(dx) < minDistX && std::abs(dz) < minDistZ) {
      return false;
    }
  }

  // ここまでの条件を満たしたので配置可能
  return true;
}

// 内部関数：中心近くと外周を避けたリング領域からランダム位置を生成する
Vector3
EnemyWallManager::GenerateRandomPosition(const Vector3 &enemyPos) const {
  (void)enemyPos; // 現状は enemyPos は使わないが、将来拡張のために残している

  // 内側と外側の半径を決めて、その間をランダムに選ぶ
  float innerRadius = 5.0f;
  float outerRadius = stageRadius_ - 3.0f;

  // 角度と半径をランダムに決めて XZ 平面上の点を作る
  float angle = RandomRange(0.0f, 6.28318530718f);
  float radius = RandomRange(innerRadius, outerRadius);

  Vector3 pos;
  pos.x = stageCenter_.x + std::cos(angle) * radius;
  pos.y = stageCenter_.y;
  pos.z = stageCenter_.z + std::sin(angle) * radius;
  return pos;
}
