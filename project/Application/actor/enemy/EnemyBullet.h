#pragma once

#include "3D/ObjClass.h"
#include "camera/Camera.h"
#include "math/Vector3.h"

#include <memory>
#include <vector>

// 敵壁前方宣言
class EnemyWallManager;

struct EnemyBullet {
  Vector3 position;
  Vector3 velocity;
  float radius;
  float lifeTime;
  bool active;

  EnemyBullet()
      : position(0.0f, 0.0f, 0.0f), velocity(0.0f, 0.0f, 0.0f), radius(0.5f),
        lifeTime(0.0f), active(false) {}
};

class EnemyBulletManager {
public:
  EnemyBulletManager();

  // ステージ情報とカメラをセットする
  void Initialize(Camera *camera, const Vector3 &stageCenter,
                  float stageRadius);

  void Update(float deltaTime);
  void Draw();

  // 壁との当たり判定
  void ResolveBulletWallCollision(EnemyWallManager &wallManager);

  // 敵位置 origin から、target（プレイヤー）に向かって1発撃つ
  void SpawnBulletAimed(const Vector3 &origin, const Vector3 &target);

  // フェーズ2用：origin から target 方向に count 発を扇状に撃つ
  void SpawnBulletSpread(const Vector3 &origin, const Vector3 &target,
                         int count, float spreadAngleRad);

  // 円（プレイヤーなど）との当たり判定
  // 当たっている弾のインデックスを返す（なければ -1）
  int CheckCollisionCircle(const Vector3 &center, float radius) const;

  // 当たった弾を消す
  void OnHitBullet(int bulletIndex);

  // 最大弾数と弾速の設定
  void SetMaxBulletCount(int maxCount) { maxBulletCount_ = maxCount; }
  void SetBulletSpeed(float speed) { bulletSpeed_ = speed; }

  // モデルポインタを外部からセットする（Enemy から渡す）
  void SetModel(ObjClass *model) { bulletModel_ = model; }

private:
  Camera *camera_ = nullptr;

  Vector3 stageCenter_{0.0f, 0.0f, 0.0f};
  float stageRadius_ = 50.0f;

  float bulletSpeed_ = 10.0f;   // 弾速
  float bulletLifeTime_ = 5.0f; // 寿命
  int maxBulletCount_ = 64;     // 最大弾数

  // 弾データ
  std::vector<EnemyBullet> bullets_;

  // 共有モデル（1個だけ）
  ObjClass *bulletModel_ = nullptr;

  int FindFreeIndex();
  static float LengthXZ(const Vector3 &v);
  static float DistanceXZ(const Vector3 &a, const Vector3 &b);

  // 実際の移動方向から弾を1発発生させる内部関数
  void SpawnBulletWithDirection(const Vector3 &origin,
                                const Vector3 &dirNormalized);
};
