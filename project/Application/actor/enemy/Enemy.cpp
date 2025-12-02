// Enemy.cpp
#include "Enemy.h"

#include <random>

namespace {
std::mt19937 &GetRngEnemy() {
  static std::random_device rd;
  static std::mt19937 mt(rd());
  return mt;
}

float RandomRangeEnemy(float minValue, float maxValue) {
  std::uniform_real_distribution<float> dist(minValue, maxValue);
  return dist(GetRngEnemy());
}

// min〜max の整数乱数（両端含む）
int RandomIntEnemy(int minValue, int maxValue) {
  std::uniform_int_distribution<int> dist(minValue, maxValue);
  return dist(GetRngEnemy());
}
} // namespace

Enemy::Enemy() {}

void Enemy::Initialize(Camera *camera, const Vector3 &spawnPos,
                       EnemyWallManager *wallManager,
                       EnemyBulletManager *bulletManager) {
  camera_ = camera;

  model_ = std::make_unique<ObjClass>();
  // ★ 敵モデル名は環境に合わせて変更OK
  model_->Initialize(camera_, "player.obj");

  transform_.translate = spawnPos;
  transform_.scale = {1.0f, 1.0f, 1.0f};

  enemyWall_ = wallManager;
  bulletManager_ = bulletManager;
  hp_ = 100;

  ResetActionTimer();

  if (model_) {
    model_->SetPosition(transform_.translate);
    model_->Update();
  }
}

void Enemy::Update(float deltaTime, const Vector3 &playerPos) {
  // 何もできないなら早期リターン
  if (!enemyWall_ && !bulletManager_) {
    return;
  }

  actionTimer_ -= deltaTime;
  if (actionTimer_ <= 0.0f) {
    // ★ タイマーが0になったので、ここでどちらか1つ行動する

    bool canWall = (enemyWall_ != nullptr);
    bool canBullet = (bulletManager_ != nullptr);

    if (canWall && canBullet) {
      // 壁 or 弾 をランダムに選択
      int r = RandomIntEnemy(0, 1); // 0:壁, 1:弾

      if (r == 0) {
        enemyWall_->SpawnWalls(transform_.translate);
      } else {
        bulletManager_->SpawnBulletAimed(transform_.translate, playerPos);
      }

    } else if (canWall) {
      // 壁だけ使える場合
      enemyWall_->SpawnWalls(transform_.translate);

    } else if (canBullet) {
      // 弾だけ使える場合
      bulletManager_->SpawnBulletAimed(transform_.translate, playerPos);
    }

    // 次の行動までの時間を設定
    ResetActionTimer();
  }

  // 将来、敵を動かしたくなったらここで transform_.translate をいじる

  if (model_) {
    model_->SetPosition(transform_.translate);
    model_->Update();
  }
}

void Enemy::Draw() {
  if (model_) {
    model_->Draw();
  }

  // 壁の描画（壁マネージャが描画を持っている想定）
  if (enemyWall_) {
    enemyWall_->Draw();
  }

  // 弾の描画は GameScene 側で enemyBulletManager_.Draw() を呼ぶ想定
}

void Enemy::ResetActionTimer() {
  actionTimer_ = RandomRangeEnemy(actionIntervalMin_, actionIntervalMax_);
}
