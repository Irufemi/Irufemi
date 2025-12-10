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
  warningModels_.clear();

  // 最大個数分の容量を事前確保しておく
  walls_.reserve(maxWallCount_);
  models_.reserve(maxWallCount_);
  warningModels_.reserve(maxWallCount_);
}

void EnemyWallManager::Update(float deltaTime) {
  // すべての壁の寿命を更新し、寿命が尽きたら無効化する
  for (auto &wall : walls_) {
    if (!wall.active) {
      continue;
    }

    // 予告→落下→着地の処理
    if (!wall.hasLanded) {
      // まだ予告中（赤い円だけ表示したいフェーズ）
      if (wall.warningTime > 0.0f) {
        wall.warningTime -= deltaTime;
        if (wall.warningTime <= 0.0f) {
          wall.warningTime = 0.0f;
          // ここから落下を開始する
          wall.fallTimer = wall.fallDuration;
          wall.position.y = wall.groundY + wall.fallStartHeight;
        }
      }
      // 落下中
      else if (wall.fallTimer > 0.0f) {
        wall.fallTimer -= deltaTime;
        float t = 1.0f - (wall.fallTimer / wall.fallDuration);
        if (t < 0.0f) {
          t = 0.0f;
        }
        if (t > 1.0f) {
          t = 1.0f;
        }
        // 上から groundY まで線形補間
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

  // 現在アクティブな壁の数を数える（必要ならデバッグに使う）
  int activeCount = 0;
  for (const auto &w : walls_) {
    if (w.active) {
      ++activeCount;
    }
  }
  (void)activeCount;
}

void EnemyWallManager::Draw() {
  // 壁とモデルは同じ数並びで対応している前提
  const int count = static_cast<int>(walls_.size());
  for (int i = 0; i < count; ++i) {
    const EnemyWall &wall = walls_[i];
    if (!wall.active) {
      continue;
    }

    ObjClass *wallModel = nullptr;
    if (i < static_cast<int>(models_.size())) {
      wallModel = models_[i].get();
    }

    ObjClass *warningModel = nullptr;
    if (i < static_cast<int>(warningModels_.size())) {
      warningModel = warningModels_[i].get();
    }

    Vector3 wallRotate{0.0f, wall.yawRadians, 0.0f};

    // ============================
    // 影(予測マーカー)の描画
    // ============================
    // ・hasLanded == false の間だけ描画する
    // ・y は常に groundY 付近に固定
    // ・スケール + α を「落下の進み具合」に合わせて変化させる
    if (warningModel && !wall.hasLanded) {

      // 影は常に地面付近
      Vector3 shadowPos = wall.position;
      shadowPos.y = wall.groundY + 0.05f; // ほんの少しだけ浮かせる

      // ---- スケール計算 ----
      const float kMinScale = 0.3f; // 落ち始める前の大きさ
      const float kMaxScale = 1.0f; // 着地直前の大きさ

      float scaleFactor = kMinScale;

      // 予告時間中：まだ最小スケール + 最小α で固定
      if (wall.warningTime > 0.0f) {
        scaleFactor = kMinScale;
      }
      // 落下中：fallTimer を使って 0→1 の補間係数を作る
      else if (wall.fallDuration > 0.0f) {
        float t = 1.0f - (wall.fallTimer / wall.fallDuration); // 0→1
        if (t < 0.0f) {
          t = 0.0f;
        }
        if (t > 1.0f) {
          t = 1.0f;
        }

        // t が進むほど影が大きく
        scaleFactor = kMinScale + (kMaxScale - kMinScale) * t;
      }

      // 影のスケール（XZ 平面メッシュ前提）
      Vector3 shadowScale(wall.halfSizeX * scaleFactor, // X方向に伸ばす
                          1.0f,                         // 厚みは 1
                          wall.halfSizeZ * scaleFactor  // Z方向に伸ばす
      );

      warningModel->SetPosition(shadowPos);
      warningModel->SetScale(shadowScale);
      warningModel->SetRotate(wallRotate);

      warningModel->Update();
      warningModel->Draw();
    }

    // ============================
    // 壁本体の描画
    // ============================
    // 予告時間中は壁を見せたくないので、warningTime が残っている間は描画しない
    if (wallModel && wall.warningTime <= 0.0f) {
      wallModel->SetPosition(wall.position);

      // 元モデルの半径（1x1x1 の cube を想定）
      const float baseHalfX = 0.5f;
      const float baseHalfZ = 0.5f;

      // halfSize に応じてスケールを計算
      float scaleX = wall.halfSizeX / baseHalfX; // 0.5 → 1.0, 1.5 → 3.0
      float scaleZ = wall.halfSizeZ / baseHalfZ; // 0.5 → 1.0

      Vector3 wallScale(scaleX, 1.0f, scaleZ);

      wallModel->SetScale(wallScale);

      // ライン壁用の Y 回転も反映
      Vector3 wallRotate{0.0f, wall.yawRadians, 0.0f};
      wallModel->SetRotate(wallRotate);

      wallModel->Update();
      wallModel->Draw();
    }

  }
}

// 壁生成アクション：敵位置を中心に、同時に最大3つまで生成を試みる
void EnemyWallManager::SpawnWalls(const Vector3 &enemyPos,
                                  const Vector3 &playerPos) {
  // すでにアクティブな壁の数を数える
  int currentCount = 0;
  for (const auto &w : walls_) {
    if (w.active) {
      currentCount++;
    }
  }

  const int kSpawnNum = 3;
  // 既に最大数に達しているなら何もしない
  if (currentCount >= maxWallCount_) {
    return;
  }

  // フェーズ1用の壁サイズ（1x1）
  const float candidateHalfX = 0.5f;
  const float candidateHalfZ = 0.5f;

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
      if (!CanPlaceWallAt(pos, enemyPos, playerPos, candidateHalfX,
                          candidateHalfZ)) {
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

        // 壁情報スロット追加
        walls_.emplace_back();

        // 壁本体モデル追加
        models_.emplace_back(std::make_unique<ObjClass>());
        models_.back()->Initialize(camera_, "wall.obj"); // 壁モデル

        // 予測用マーカー（赤い円など）のモデル追加
        warningModels_.emplace_back(std::make_unique<ObjClass>());
        warningModels_.back()->Initialize(camera_, "warning.obj");

        freeIndex = static_cast<int>(walls_.size()) - 1;
      }

      // 壁情報を設定してアクティブ化
      EnemyWall &w = walls_[freeIndex];
      w.position = pos;
      w.position.y = stageCenter_.y; // 基本はステージの地面高さ

      // フェーズ1の壁サイズは 1x1（半径0.5）
      w.halfSizeX = candidateHalfX;
      w.halfSizeZ = candidateHalfZ;

      w.groundY = stageCenter_.y;
      w.fallStartHeight = wallFallStartHeight_;
      w.fallDuration = wallFallDuration_;
      w.fallTimer = 0.0f;
      w.warningTime = wallWarningTime_;
      w.hasLanded = false;
      w.yawRadians = 0.0f; // フェーズ1の壁は回転なしで扱う

      // 予告＋落下＋着地後の存続時間をまとめた寿命
      w.lifeTime = wallLifeTime_ + wallFallDuration_ + wallWarningTime_;
      w.active = true;

      spawned = true;
      ++currentCount;
      break;
    }

    if (!spawned) {
      // この1個は良い場所が見つからなかったという扱い
    }
  }
}

// フェーズ2用：3個の壁を「バラバラの位置・バラバラの回転」で同時生成
void EnemyWallManager::SpawnWallLine3x1(const Vector3 &enemyPos,
                                        const Vector3 &playerPos) {
  // すでにアクティブな壁数を数える
  int currentCount = 0;
  for (const auto &w : walls_) {
    if (w.active) {
      ++currentCount;
    }
  }

  const int kSpawnNum = 3; // 同時に出したい個数
  const int kMaxTry = 20;  // 1個あたりの最大試行回数

  // フェーズ2用の壁サイズ（3x1）
  const float candidateHalfX = 1.5f;
  const float candidateHalfZ = 0.5f;

  // 上限に達していたら何もしない
  if (currentCount >= maxWallCount_) {
    return;
  }

  // 3個分トライするが、途中で上限に達したら打ち切る
  for (int i = 0; i < kSpawnNum; ++i) {
    if (currentCount >= maxWallCount_) {
      break;
    }

    bool spawned = false;

    // この1個を置くために、最大 kMaxTry 回ランダム位置を試す
    for (int t = 0; t < kMaxTry; ++t) {
      // ステージ上のランダム位置（リング内）を生成
      Vector3 pos = GenerateRandomPosition(enemyPos);

      // 敵の近さ・プレイヤーの近さ・他の壁との重なりなどをチェック
      if (!CanPlaceWallAt(pos, enemyPos, playerPos, candidateHalfX,
                          candidateHalfZ)) {
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
          // もう増やせない
          spawned = false;
          break;
        }

        // 壁情報スロット追加
        walls_.emplace_back();

        // 壁本体モデル追加
        models_.emplace_back(std::make_unique<ObjClass>());
        models_.back()->Initialize(camera_, "wall.obj");

        // 予測用マーカー（影）のモデル追加
        warningModels_.emplace_back(std::make_unique<ObjClass>());
        warningModels_.back()->Initialize(camera_, "warning.obj");

        freeIndex = static_cast<int>(walls_.size()) - 1;
      }

      // 壁情報を設定してアクティブ化
      EnemyWall &w = walls_[freeIndex];
      w.position = pos;
      w.position.y = stageCenter_.y; // 地面高さに合わせる

      // 当たり判定サイズ（3x1 の壁）
      w.halfSizeX = candidateHalfX;
      w.halfSizeZ = candidateHalfZ;

      // 落下・予告用
      w.groundY = stageCenter_.y;
      w.fallStartHeight = wallFallStartHeight_;
      w.fallDuration = wallFallDuration_;
      w.fallTimer = 0.0f;
      w.warningTime = wallWarningTime_;
      w.hasLanded = false;

      // 一個ごとにランダムな Y 回転
      w.yawRadians = RandomRange(0.0f, 6.28318530718f);

      // 予告＋落下＋着地後の存続時間をまとめた寿命
      w.lifeTime = wallLifeTime_ + wallFallDuration_ + wallWarningTime_;
      w.active = true;

      spawned = true;
      ++currentCount;
      break;
    }

    // 場所が見つからなくて spawned=false の場合は、その1個は諦める
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
    // 着地前は当たり判定なし（予告中＆落下中も含める）
    if (!w.hasLanded) {
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
                                      const Vector3 &enemyPos,
                                      const Vector3 &playerPos,
                                      float candidateHalfX,
                                      float candidateHalfZ) const {
  // ステージ外周から少し内側の範囲だけを使う
  float margin = 1.0f;

  // 候補壁の「半径」（対角線の半分）
  float candidateRadius = std::sqrt(candidateHalfX * candidateHalfX +
                                    candidateHalfZ * candidateHalfZ);

  float distFromCenter = DistanceXZ(pos, stageCenter_);
  if (distFromCenter + candidateRadius > stageRadius_ - margin) {
    return false;
  }

  // 敵の近くには配置しないようにする（候補の半径も考慮）
  float avoidEnemyRadius = 5.0f + candidateRadius;
  if (DistanceXZ(pos, enemyPos) < avoidEnemyRadius) {
    return false;
  }

  // プレイヤーの近くには配置しないようにする（候補の半径も考慮）
  float avoidPlayerRadius = 5.0f + candidateRadius;
  if (DistanceXZ(pos, playerPos) < avoidPlayerRadius) {
    return false;
  }

  // 既存の壁と近すぎないかチェックする
  for (const auto &w : walls_) {
    if (!w.active) {
      continue;
    }

    float dx = pos.x - w.position.x;
    float dz = pos.z - w.position.z;

    // 候補壁と既存壁の半サイズ＋マージン
    float minDistX = candidateHalfX + w.halfSizeX + 0.5f;
    float minDistZ = candidateHalfZ + w.halfSizeZ + 0.5f;

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
