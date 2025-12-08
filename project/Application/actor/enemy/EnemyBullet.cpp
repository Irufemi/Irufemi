#include "EnemyBullet.h"
#include "EnemyWall.h"
#include <cmath>
#include <random>

// 弾用の乱数・ヘルパー
namespace {

// 弾用の乱数生成器を返す（static で一度だけ初期化）
std::mt19937 &GetRngBullet() {
  static std::random_device rd;
  static std::mt19937 mt(rd());
  return mt;
}

// min〜max の範囲でランダムな浮動小数を返す
float RandomRangeBullet(float minValue, float maxValue) {
  std::uniform_real_distribution<float> dist(minValue, maxValue);
  return dist(GetRngBullet());
}
} // namespace

EnemyBulletManager::EnemyBulletManager() {}

void EnemyBulletManager::Initialize(Camera *camera, const Vector3 &stageCenter,
                                    float stageRadius) {
  // カメラとステージの中心・半径を記録しておく
  camera_ = camera;
  stageCenter_ = stageCenter;
  stageRadius_ = stageRadius;

  // 弾の情報とモデル配列を初期化
  bullets_.clear();
  models_.clear();

  // 最大弾数分の容量を事前確保しておく
  bullets_.reserve(maxBulletCount_);
  models_.reserve(maxBulletCount_);
}

int EnemyBulletManager::FindFreeIndex() {
  // 既存配列の中から非アクティブな弾のスロットを探す
  for (int i = 0; i < static_cast<int>(bullets_.size()); ++i) {
    if (!bullets_[i].active) {
      return i;
    }
  }

  // まだ最大弾数に達していなければ、新しい弾とモデルを作る
  if (static_cast<int>(bullets_.size()) >= maxBulletCount_) {
    return -1;
  }

  bullets_.emplace_back();
  models_.emplace_back(std::make_unique<ObjClass>());

  // 弾のモデルを初期化（名前は環境に合わせて変更してもよい）
  models_.back()->Initialize(camera_, "bullet.obj");

  // 新しく追加したスロットのインデックスを返す
  return static_cast<int>(bullets_.size()) - 1;
}

float EnemyBulletManager::LengthXZ(const Vector3 &v) {
  // XZ 平面上での長さ（Yは無視）を計算する
  return std::sqrt(v.x * v.x + v.z * v.z);
}

float EnemyBulletManager::DistanceXZ(const Vector3 &a, const Vector3 &b) {
  // 2点間の XZ 平面距離を計算する
  float dx = a.x - b.x;
  float dz = a.z - b.z;
  return std::sqrt(dx * dx + dz * dz);
}

void EnemyBulletManager::SpawnBulletAimed(const Vector3 &origin,
                                          const Vector3 &target) {
  // 空いている弾スロットを取得（なければ何もしない）
  int index = FindFreeIndex();
  if (index < 0) {
    return;
  }

  // 新しい弾の参照を取り、初期位置を設定
  EnemyBullet &b = bullets_[index];
  b.position = origin;

  // XZ 平面でターゲット方向ベクトルを作る
  Vector3 dir{};
  dir.x = target.x - origin.x;
  dir.y = 0.0f;
  dir.z = target.z - origin.z;

  // 正規化して単位ベクトルにする
  float len = LengthXZ(dir);
  if (len > 0.0001f) {
    dir.x /= len;
    dir.z /= len;
  } else {
    // ほぼ同じ位置にいる場合はランダム方向にする
    float angle = RandomRangeBullet(0.0f, 6.28318530718f);
    dir.x = std::cos(angle);
    dir.z = std::sin(angle);
  }

  // 方向ベクトルに速度を掛けて弾速ベクトルを設定
  b.velocity.x = dir.x * bulletSpeed_;
  b.velocity.y = 0.0f;
  b.velocity.z = dir.z * bulletSpeed_;

  // 当たり判定用の半径や寿命、アクティブ状態を設定
  b.radius = 0.5f;
  b.lifeTime = bulletLifeTime_;
  b.active = true;
}

void EnemyBulletManager::Update(float deltaTime) {
  // すべての弾を走査して更新する
  for (auto &b : bullets_) {
    if (!b.active) {
      continue;
    }

    // 速度に応じて位置を更新
    b.position.x += b.velocity.x * deltaTime;
    b.position.y += b.velocity.y * deltaTime;
    b.position.z += b.velocity.z * deltaTime;

    // 残り寿命を減らし、尽きたら非アクティブ化
    b.lifeTime -= deltaTime;
    if (b.lifeTime <= 0.0f) {
      b.active = false;
      continue;
    }

    // ステージの中心から一定距離以上離れたら消す
    float dist = DistanceXZ(b.position, stageCenter_);
    if (dist > stageRadius_ + 5.0f) {
      b.active = false;
      continue;
    }
  }
}

void EnemyBulletManager::Draw() {
  // アクティブな弾だけモデルを更新・描画する
  const int count = static_cast<int>(bullets_.size());
  for (int i = 0; i < count; ++i) {
    const EnemyBullet &b = bullets_[i];
    if (!b.active) {
      continue;
    }

    ObjClass *model = models_[i].get();
    if (!model) {
      continue;
    }

    // モデルに位置を反映してから描画
    model->SetPosition(b.position);
    model->Update();
    model->Draw();
  }
}

int EnemyBulletManager::CheckCollisionCircle(const Vector3 &center,
                                             float radius) const {
  // 引数で渡された円と、すべての弾との当たり判定を行う
  const float rBase = radius;

  for (int i = 0; i < static_cast<int>(bullets_.size()); ++i) {
    const EnemyBullet &b = bullets_[i];
    if (!b.active) {
      continue;
    }

    // XZ 平面での距離の二乗を計算
    float dx = center.x - b.position.x;
    float dz = center.z - b.position.z;
    float distSq = dx * dx + dz * dz;

    // プレイヤー円＋弾円の合計半径
    float r = rBase + b.radius;
    float rSq = r * r;

    // 距離の二乗が半径の二乗以下なら衝突
    if (distSq <= rSq) {
      return i;
    }
  }

  // どの弾とも当たっていない
  return -1;
}

void EnemyBulletManager::OnHitBullet(int bulletIndex) {
  // インデックスが有効かチェック（範囲外は無視）
  if (bulletIndex < 0 || bulletIndex >= static_cast<int>(bullets_.size())) {
    return;
  }

  EnemyBullet &b = bullets_[bulletIndex];
  if (!b.active) {
    return;
  }

  // 当たった弾を非アクティブにして消す
  b.active = false;
}

// 敵弾と壁の当たり判定
void EnemyBulletManager::ResolveBulletWallCollision(EnemyWallManager& wallManager) {
    for (size_t i = 0; i < bullets_.size(); ++i) {
        EnemyBullet& b = bullets_[i];
        if (!b.active) {
            continue;
        }

        // 弾を円として壁 AABB と判定
        int hitWallIndex = wallManager.CheckCollisionCircle(b.position, b.radius);
        if (hitWallIndex >= 0) {
            // 弾と壁を両方消す
            OnHitBullet(static_cast<int>(i));
            wallManager.OnEnemyHitWall(hitWallIndex);
        }
    }
}