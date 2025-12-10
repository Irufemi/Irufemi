#include "EnemyBullet.h"
#include "Application/actor/enemy/EnemyWall.h"
#include "Application/camera/Camera.h"
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

  // ロジック用の弾配列
  bullets_.clear();
  bullets_.resize(maxBulletCount_);
  for (int i = 0; i < maxBulletCount_; ++i) {
    bullets_[i] = EnemyBullet();
    bullets_[i].active = false;
  }

  // 描画用の Region を初期化（bullet.obj を1回だけロード）
  bulletRegion_.Initialize(camera_, "bullet.obj");
}

int EnemyBulletManager::FindFreeIndex() {
  for (int i = 0; i < static_cast<int>(bullets_.size()); ++i) {
    if (!bullets_[i].active) {
      return i;
    }
  }
  return -1;
}

float EnemyBulletManager::LengthXZ(const Vector3 &v) {
  return std::sqrt(v.x * v.x + v.z * v.z);
}

float EnemyBulletManager::DistanceXZ(const Vector3 &a, const Vector3 &b) {
  float dx = a.x - b.x;
  float dz = a.z - b.z;
  return std::sqrt(dx * dx + dz * dz);
}

void EnemyBulletManager::SpawnBulletWithDirection(
    const Vector3 &origin, const Vector3 &dirNormalized) {
  int index = FindFreeIndex();
  if (index < 0) {
    return;
  }

  EnemyBullet &b = bullets_[index];
  b.position = origin;
  b.velocity.x = dirNormalized.x * bulletSpeed_;
  b.velocity.y = 0.0f;
  b.velocity.z = dirNormalized.z * bulletSpeed_;
  b.radius = 0.5f;
  b.lifeTime = bulletLifeTime_;
  b.active = true;
}

void EnemyBulletManager::SpawnBulletAimed(const Vector3 &origin,
                                          const Vector3 &target) {
  Vector3 dir{};
  dir.x = target.x - origin.x;
  dir.y = 0.0f;
  dir.z = target.z - origin.z;

  float len = LengthXZ(dir);
  if (len > 0.0001f) {
    dir.x /= len;
    dir.z /= len;
  } else {
    float angle = RandomRangeBullet(0.0f, 6.28318530718f);
    dir.x = std::cos(angle);
    dir.z = std::sin(angle);
  }

  SpawnBulletWithDirection(origin, dir);
}

void EnemyBulletManager::SpawnBulletSpread(const Vector3 &origin,
                                           const Vector3 &target, int count,
                                           float spreadAngleRad) {
  if (count <= 0) {
    return;
  }

  Vector3 baseDir{};
  baseDir.x = target.x - origin.x;
  baseDir.y = 0.0f;
  baseDir.z = target.z - origin.z;

  float len = LengthXZ(baseDir);
  if (len > 0.0001f) {
    baseDir.x /= len;
    baseDir.z /= len;
  } else {
    float angle = RandomRangeBullet(0.0f, 6.28318530718f);
    baseDir.x = std::cos(angle);
    baseDir.z = std::sin(angle);
  }

  float centerIndex = static_cast<float>(count - 1) * 0.5f;

  for (int i = 0; i < count; ++i) {
    float offsetIndex = static_cast<float>(i) - centerIndex;
    float angle = offsetIndex * spreadAngleRad;

    Vector3 dir{};
    float c = std::cos(angle);
    float s = std::sin(angle);
    dir.x = baseDir.x * c - baseDir.z * s;
    dir.y = 0.0f;
    dir.z = baseDir.x * s + baseDir.z * c;

    float len2 = LengthXZ(dir);
    if (len2 > 0.0001f) {
      dir.x /= len2;
      dir.z /= len2;
    }

    SpawnBulletWithDirection(origin, dir);
  }
}

void EnemyBulletManager::Update(float deltaTime) {
  // ロジック更新
  for (auto &b : bullets_) {
    if (!b.active) {
      continue;
    }

    b.position.x += b.velocity.x * deltaTime;
    b.position.y += b.velocity.y * deltaTime;
    b.position.z += b.velocity.z * deltaTime;

    b.lifeTime -= deltaTime;
    if (b.lifeTime <= 0.0f) {
      b.active = false;
      continue;
    }

    float dist = DistanceXZ(b.position, stageCenter_);
    if (dist > stageRadius_ + 5.0f) {
      b.active = false;
      continue;
    }
  }

  // 描画用のインスタンス情報を構築
  bulletRegion_.ClearInstances();

  for (const auto &b : bullets_) {
    if (!b.active) {
      continue;
    }

    Transform t;
    t.translate = b.position;
    t.rotate = {0.0f, 0.0f, 0.0f};
    t.scale = {1.0f, 1.0f, 1.0f};

    bulletRegion_.AddInstance(t);
  }
}

void EnemyBulletManager::Draw() { bulletRegion_.Draw(); }

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

void EnemyBulletManager::ResolveBulletWallCollision(
    EnemyWallManager &wallManager) {
  for (size_t i = 0; i < bullets_.size(); ++i) {
    EnemyBullet &b = bullets_[i];
    if (!b.active) {
      continue;
    }

    int hitWallIndex = wallManager.CheckCollisionCircle(b.position, b.radius);
    if (hitWallIndex >= 0) {
      OnHitBullet(static_cast<int>(i));
      wallManager.OnEnemyHitWall(hitWallIndex);
    }
  }
}
