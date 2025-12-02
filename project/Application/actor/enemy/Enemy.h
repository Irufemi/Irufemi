#include "3D/ObjClass.h"
#include "camera/Camera.h"
#include "math/Transform.h"

class Wall {
public:
  bool active = false;
  Transform transform{
      {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
  Vector3 size = {4.0f, 4.0f, 0.3f};
};

class Bullet {
public:
  bool active = false;
  Transform transform{
      {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
  Vector3 vel;
  float speed = 0.2f;
};

enum class EnemyState {
  Idle,     // 待機状態
  Shoot,    // 弾を撃つ状態
  MakeWall, // 壁生成状態
  Relocate, // 潜り移動状態
  Dead      // 死亡状態
};

class Enemy {
public:
  Enemy();
  ~Enemy();
  void Initialize(Camera *cam);
  void Update(float dt, const Vector3 &playerPos);
  void Draw();

public:
  bool HasAnyWall() const;

private:
  Transform transform_;
  float hp_ = 100.0f;

  Camera *camera_ = nullptr;

  EnemyState state_ = EnemyState::Idle;
  float stateTimer_ = 0.0f;

  // --- 行動クールダウン（共通タイマー）---
  float actionTimer_ = 0.0f;       // 次の行動までの残り時間
  float actionIntervalMin_ = 1.5f; // 最短クールタイム
  float actionIntervalMax_ = 3.5f; // 最長クールタイム

  // --- 弾 ---
  bool bulletActive_ = false; // 弾が存在するかどうか

  // --- 壁 ---
  int wallCount_ = 0;     // 生成した壁の数
  int maxWallCount_ = 3;  // 最大生成可能な壁の数
  bool wallActive_[10]{}; // 壁が存在するかどうか

  // --- 敵が直接管理する壁と弾 ---
  Wall wall_[3];
  Bullet bullet_;

  std::unique_ptr<ObjClass> enemyModel_ = nullptr;

  std::unique_ptr<ObjClass> bulletModel_ = nullptr;

  std::unique_ptr<ObjClass> wallModel_ = nullptr;

  void ChangeState(EnemyState s);                       // 状態遷移
  void Idle(float dt, const Vector3 &playerPos);        // 待機状態
  void BulletShoot(float dt, const Vector3 &playerPos); // 弾撃つ状態
  void MakeWall(float dt, const Vector3 &playerPos);    // 壁生成状態
  void Relocate(float dt);                              // 移動する状態
  void ResetActionTimer();                              // 行動タイマーリセット
};
