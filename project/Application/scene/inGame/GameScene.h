#pragma once

#include "scene/IScene.h"

#include <memory>

#include "2D/Circle2D.h"
#include "2D/NumberText.h"
#include "2D/Sprite.h"
#include "2D/TimeDisplay.h"
#include "3D/CylinderClass.h"
#include "3D/ObjClass.h"
#include "3D/PointLightClass.h"
#include "3D/Region.h"
#include "3D/SphereClass.h"
#include "3D/SpotLightClass.h"
#include "3D/TriangleClass.h"
#include "3D/particle/ParticleSystem.h"
#include "actor/enemy/Enemy.h"
#include "actor/enemy/EnemyBullet.h"
#include "actor/enemy/EnemyWall.h"
#include "actor/player/Player.h"
#include "actor/rock/RockManager.h"
#include "audio/Bgm.h"
#include "stage/field/field.h"

// 前方宣言
class IrufemiEngine;

class InputManager;

class Camera;
class DebugCamera;

/// <summary>
/// ゲーム
/// </summary>
class GameScene : public IScene {
private: // 関数
private: // 変数(ゲーム)
         // ----- プレイヤー -----
  std::unique_ptr<SphereClass> playerObj_ = nullptr;
  std::unique_ptr<Player> player_ = nullptr;

  // 1フレーム前のプレイヤー位置
  Vector3 prevPlayerPos_{};

  // ----- エネミー -----
  std::unique_ptr<ObjClass> enemyObj_ = nullptr;
  std::unique_ptr<Enemy> enemy_ = nullptr;

  EnemyWallManager enemyWallManager_;     // 敵が使う壁マネージャ
  EnemyBulletManager enemyBulletManager_; // 敵が使う弾マネージャー

  // 岩マネージャ
  std::unique_ptr<RockManager> rockManager_ = nullptr;

  // フィールド
  Field field_;

private: // メンバ変数(システム)
  // カメラ
  std::unique_ptr<Camera> camera_ = nullptr;

  // デバッグカメラ
  std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

  std::unique_ptr<PointLightClass> pointLight_ = nullptr;

  std::unique_ptr<SpotLightClass> spotLight_ = nullptr;

  int loadTexture = false;

  bool debugMode = false;

  // ポインタ参照

  // エンジン
  IrufemiEngine *engine_ = nullptr;

public: // メンバ関数
  // デストラクタ
  ~GameScene();

  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(IrufemiEngine *engine) override;

  /// <summary>
  /// 更新
  /// </summary>
  void Update() override;

  /// <summary>
  /// 描画
  /// </summary>
  void Draw() override;

private:
  /// <summary>
  /// すべての当たり判定を取る
  /// </summary>
  void DoCollision();

  /// <summary>
  /// 直前フレームの移動方向をもとにノックバックを与える
  /// </summary>
  Vector3 ApplyPlayerKnockback(const float knockbackPower);
};
