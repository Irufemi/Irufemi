// EnemyBullet.cpp
#include "EnemyBullet.h"

#include <cmath>
#include <random>

namespace {
std::mt19937 &GetRngBullet() {
  static std::random_device rd;
  static std::mt19937 mt(rd());
  return mt;
}

float RandomRangeBullet(float minValue, float maxValue) {
  std::uniform_real_distribution<float> dist(minValue, maxValue);
  return dist(GetRngBullet());
}
} // namespace

EnemyBulletManager::EnemyBulletManager() {}

void EnemyBulletManager::Initialize(Camera *camera, const Vector3 &stageCenter,
                                    float stageRadius) {
  camera_ = camera;
  stageCenter_ = stageCenter;
  stageRadius_ = stageRadius;

  bullets_.clear();
  models_.clear();

  bullets_.reserve(maxBulletCount_);
  models_.reserve(maxBulletCount_);
}

int EnemyBulletManager::FindFreeIndex() {
  for (int i = 0; i < static_cast<int>(bullets_.size()); ++i) {
    if (!bullets_[i].active) {
      return i;
    }
  }

  if (static_cast<int>(bullets_.size()) >= maxBulletCount_) {
    return -1;
  }

  bullets_.emplace_back();
  models_.emplace_back(std::make_unique<ObjClass>());

  // ★ 弾のモデル名は環境に合わせて変えてOK
  //   "sphere.obj" が無ければ "cube.obj" とかにする
  models_.back()->Initialize(camera_, "bullet.obj");

  return static_cast<int>(bullets_.size()) - 1;
}

float EnemyBulletManager::LengthXZ(const Vector3 &v) {
  return std::sqrt(v.x * v.x + v.z * v.z);
}

float EnemyBulletManager::DistanceXZ(const Vector3 &a, const Vector3 &b) {
  float dx = a.x - b.x;
  float dz = a.z - b.z;
  return std::sqrt(dx * dx + dz * dz);
}

void EnemyBulletManager::SpawnBulletAimed(const Vector3 &origin,
                                          const Vector3 &target) {
  int index = FindFreeIndex();
  if (index < 0) {
    return;
  }

  EnemyBullet &b = bullets_[index];
  b.position = origin;

  // XZ平面でターゲット方向
  Vector3 dir{};
  dir.x = target.x - origin.x;
  dir.y = 0.0f;
  dir.z = target.z - origin.z;

  float len = LengthXZ(dir);
  if (len > 0.0001f) {
    dir.x /= len;
    dir.z /= len;
  } else {
    // 同じ位置ならランダム方向
    float angle = RandomRangeBullet(0.0f, 6.28318530718f);
    dir.x = std::cos(angle);
    dir.z = std::sin(angle);
  }

  b.velocity.x = dir.x * bulletSpeed_;
  b.velocity.y = 0.0f;
  b.velocity.z = dir.z * bulletSpeed_;

  b.radius = 0.5f;
  b.lifeTime = bulletLifeTime_;
  b.active = true;
}

void EnemyBulletManager::Update(float deltaTime) {
  for (auto &b : bullets_) {
    if (!b.active) {
      continue;
    }

    // 位置更新
    b.position.x += b.velocity.x * deltaTime;
    b.position.y += b.velocity.y * deltaTime;
    b.position.z += b.velocity.z * deltaTime;

    // 寿命
    b.lifeTime -= deltaTime;
    if (b.lifeTime <= 0.0f) {
      b.active = false;
      continue;
    }

    // ステージ外に出たら消す
    float dist = DistanceXZ(b.position, stageCenter_);
    if (dist > stageRadius_ + 5.0f) {
      b.active = false;
      continue;
    }
  }
}

void EnemyBulletManager::Draw() {
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

    model->SetPosition(b.position);
    model->Update();
    model->Draw();
  }
}

int EnemyBulletManager::CheckCollisionCircle(const Vector3 &center,
                                             float radius) const {
  const float rBase = radius;

  for (int i = 0; i < static_cast<int>(bullets_.size()); ++i) {
    const EnemyBullet &b = bullets_[i];
    if (!b.active) {
      continue;
    }

    float dx = center.x - b.position.x;
    float dz = center.z - b.position.z;
    float distSq = dx * dx + dz * dz;
    float r = rBase + b.radius;
    float rSq = r * r;

    if (distSq <= rSq) {
      return i;
    }
  }

  return -1;
}

void EnemyBulletManager::OnHitBullet(int bulletIndex) {
  if (bulletIndex < 0 || bulletIndex >= static_cast<int>(bullets_.size())) {
    return;
  }

  EnemyBullet &b = bullets_[bulletIndex];
  if (!b.active) {
    return;
  }

  b.active = false;
}
