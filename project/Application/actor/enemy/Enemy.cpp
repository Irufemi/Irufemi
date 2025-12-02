#include "Enemy.h"
#include "function/Math.h"
#include <array>
#include <cmath>
#include <cstdlib>

// 壁の残り寿命（秒）。index ごとに管理（当たりで消えても寿命は自然減衰）
static std::array<float, 3> wallLifeRemaining_{0.0f, 0.0f, 0.0f};
// 壁の寿命（同時生成されたグループに割り当てる既定秒数）
static constexpr float kWallLifetimeSec = 5.0f;
// 壁の生成半径（敵中心からの距離）
static constexpr float kWallSpawnRadius = 8.0f;
// 壁の最小離間距離（重なり判定）
static constexpr float kWallMinSeparation =
    2.0f; // 必要に応じて壁サイズに合わせて調整

// 位置が既存のアクティブ壁と重なっていないか
static bool IsOccupiedByWall(const Vector3 &pos, const Wall (&walls)[3]) {
  for (int i = 0; i < 3; ++i) {
    if (!walls[i].active)
      continue;
    const Vector3 wp = walls[i].transform.translate;
    const float dx = pos.x - wp.x;
    const float dz = pos.z - wp.z;
    const float distSq = dx * dx + dz * dz;
    if (distSq < (kWallMinSeparation * kWallMinSeparation)) {
      return true;
    }
  }
  return false;
}

// ランダム位置を1つ生成（敵周辺円周内）、既存壁と重ならない位置を返す
static Vector3 FindRandomWallPosAround(const Vector3 &center,
                                       const Wall (&walls)[3]) {
  // 最大試行回数
  for (int tries = 0; tries < 16; ++tries) {
    float angle =
        static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    angle *= (2.0f * 3.141592654f);
    const float r = kWallSpawnRadius;
    Vector3 pos{center.x + std::cos(angle) * r, center.y,
                center.z + std::sin(angle) * r};

    if (!IsOccupiedByWall(pos, walls)) {
      return pos;
    }
  }
  // 失敗したら中心から固定オフセット（最悪回避）
  return Vector3{center.x + rintf(kWallSpawnRadius), center.y, center.z};
}

bool Enemy::HasAnyWall() const {
  for (int i = 0; i < maxWallCount_; ++i) {
    if (wall_[i].active)
      return true;
  }
  return false;
}

void Enemy::ResetActionTimer() {
  float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
  actionTimer_ =
      actionIntervalMin_ + (actionIntervalMax_ - actionIntervalMin_) * t;
}

Enemy::Enemy() {}

Enemy::~Enemy() {}

void Enemy::Initialize(Camera *cam) {
  camera_ = cam;
  transform_ = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};

  enemyModel_ = std::make_unique<ObjClass>();
  enemyModel_->Initialize(cam, "player.obj");

  bulletModel_ = std::make_unique<ObjClass>();
  bulletModel_->Initialize(cam, "axis.obj");

  wallModel_ = std::make_unique<ObjClass>();
  wallModel_->Initialize(cam, "block.obj");

  // 壁状態初期化
  for (int i = 0; i < 3; ++i) {
    wall_[i].active = false;
    wallLifeRemaining_[i] = 0.0f;
  }

  // 行動タイマー初期化
  ResetActionTimer();
}

void Enemy::ChangeState(EnemyState s) {
  state_ = s;
  stateTimer_ = 0.0f;
}

void Enemy::Update(float dt, const Vector3 &playerPos) {

  stateTimer_ += dt;

  switch (state_) {
  case EnemyState::Idle:
    Idle(dt, playerPos);
    break;
  case EnemyState::Shoot:
    BulletShoot(dt, playerPos);
    break;
  case EnemyState::MakeWall:
    MakeWall(dt, playerPos);
    break;
  case EnemyState::Relocate:
    Relocate(dt);
    break;
  }

  // 壁の寿命処理
  for (int i = 0; i < maxWallCount_; ++i) {
    if (!wall_[i].active)
      continue;
    wallLifeRemaining_[i] -= dt;
    if (wallLifeRemaining_[i] <= 0.0f) {
      wall_[i].active = false;
      wallLifeRemaining_[i] = 0.0f;
    }
  }

  // 弾の移動処理
  if (bullet_.active) {
    bullet_.transform.translate.x += bullet_.vel.x * bullet_.speed*dt;
    bullet_.transform.translate.z += bullet_.vel.z * bullet_.speed*dt;
  }
}

void Enemy::Draw() {
  // 敵本体
  enemyModel_->SetPosition(transform_.translate);
  enemyModel_->SetRotate(transform_.rotate);
  enemyModel_->SetScale(transform_.scale);
  enemyModel_->Draw();

  // 壁
  for (int i = 0; i < maxWallCount_; ++i) {
    if (wall_[i].active) {
      wallModel_->SetPosition(wall_[i].transform.translate);
      wallModel_->SetScale(wall_[i].size);
      wallModel_->Draw();
    }
  }

  // 弾
  if (bullet_.active) {
    bulletModel_->SetPosition(bullet_.transform.translate);
    bulletModel_->SetScale({1.0f, 1.0f, 1.0f}); // 目立つように
    bulletModel_->Draw();
  }
}

void Enemy::Idle(float dt, const Vector3 &playerPos) {
  // 共通クールタイムを減らす
  actionTimer_ -= dt;

  // まだ時間が残っているなら何もしない
  if (actionTimer_ > 0.0f) {
    return;
  }

  // ===== タイマーが切れたら、次の行動をランダムで1つ決める =====
  int choice = std::rand() % 3; // 0:Shoot, 1:Wall, 2:Relocate

  switch (choice) {
  case 0:
    ChangeState(EnemyState::Shoot);
    break;
  case 1:
    ChangeState(EnemyState::MakeWall);
    break;
  case 2:
    ChangeState(EnemyState::Relocate);
    break;
  }

  // ★クールダウンの再セットは「行動が終わったタイミング」でやる
  //   → Idleを抜けるだけなのでここではしない
}

void Enemy::BulletShoot(float dt, const Vector3 &playerPos) {
  if (!bullet_.active) {
    bullet_.active = true;
    bullet_.transform.translate = transform_.translate;

    Vector3 dir =
        Math::Normalize(Math::Subtract(playerPos, transform_.translate));
    bullet_.vel = dir;

    // 速度が未設定ならデフォルトを入れる
    if (bullet_.speed <= 0.0f)
      bullet_.speed = 3.0f;
  }

  // 1秒経過で終了 → Idleへ
  if (stateTimer_ > 1.0f) {
    bullet_.active = false;

    // ★次の行動までのクールタイムを設定
    ResetActionTimer();
    ChangeState(EnemyState::Idle);
  }
}

void Enemy::MakeWall(float dt, const Vector3 &playerPos) {
  // 空きスロットに最大3つ生成。同時生成は同一寿命を割り当て。
  int created = 0;
  for (int i = 0; i < maxWallCount_ && created < 3; ++i) {
    if (wall_[i].active)
      continue; // 既存壁は維持
    Vector3 pos = FindRandomWallPosAround(transform_.translate, wall_);
    if (IsOccupiedByWall(pos, wall_)) {
      // 念のためチェック（FindRandomWallPosAround で十分なはず）
      continue;
    }
    wall_[i].active = true;
    wall_[i].transform.translate = pos;

    // サイズが未設定なら標準値
    if (wall_[i].size.x <= 0.0f || wall_[i].size.y <= 0.0f ||
        wall_[i].size.z <= 0.0f) {
      wall_[i].size = Vector3{3.0f, 1.0f, 1.0f};
    }

    // 今回のバッチ寿命を付与（既存の壁とは独立）
    wallLifeRemaining_[i] = kWallLifetimeSec;
    ++created;
  }

  // この行動は即時生成後、短時間で抜けたいならここで待機せずIdleへ
  // 生成のみで十分なので即終了
  ResetActionTimer();
  ChangeState(EnemyState::Idle);
}

void Enemy::Relocate(float dt) {
  // ランダムな別位置へワープ（後から演出をつける）
  float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
  angle *= (2.0f * 3.141592654f);
  float radius = 20.0f;

  transform_.translate.x = cos(angle) * radius;
  transform_.translate.z = sin(angle) * radius;

  if (stateTimer_ > 1.0f) {
    // ★ワープ演出が終わったタイミングでリセット
    ResetActionTimer();
    ChangeState(EnemyState::Idle);
  }
}
