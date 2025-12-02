// Enemy.h
#pragma once
#include "EnemyWall.h"
#include "EnemyBullet.h"
#include "camera/Camera.h"
#include "3D/ObjClass.h"

class Enemy {
public:
  Enemy();

  // 壁マネージャと弾マネージャのポインタを渡す
  void Initialize(Camera *camera, const Vector3 &spawnPos,
                  EnemyWallManager *wallManager,
                  EnemyBulletManager *bulletManager);

  // playerPos は「弾を撃つときの狙い先」に使う
  void Update(float deltaTime, const Vector3 &playerPos);
  void Draw();

  const Vector3 &GetPosition() const { return transform_.translate; }

private:
  Camera *camera_ = nullptr;
  std::unique_ptr<ObjClass> model_ = nullptr;

  struct Transform {
    Vector3 translate{0.0f, 0.0f, 0.0f};
    Vector3 rotate{0.0f, 0.0f, 0.0f};
    Vector3 scale{1.0f, 1.0f, 1.0f};
  } transform_;

  int hp_ = 100;

  EnemyWallManager *enemyWall_ = nullptr;
  EnemyBulletManager *bulletManager_ = nullptr;

  // 行動タイマーは1つだけ
  float actionTimer_ = 0.0f;
  float actionIntervalMin_ = 2.0f; // 2〜4秒の間で行動
  float actionIntervalMax_ = 4.0f;

  void ResetActionTimer();
};