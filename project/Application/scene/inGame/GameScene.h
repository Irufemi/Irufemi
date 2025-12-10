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
#include "audio/Se.h"
#include "effect/Fade.h"
#include "stage/field/field.h"
#include "stage/skyDome/SkyDome.h"
#include "ui/RockMulti.h"
#include "ui/EnemyHpGauge.h"


// 前方宣言
class IrufemiEngine;

class InputManager;

class Camera;
class DebugCamera;
class Se;

enum class GameState {
  Tutorial,
  Playing,
  PlayerDead,
  EnemyDead,
  GameOver,
  Clear,
};

enum class TutorialState {
  Rock,
  Attack,
  Damage,
};

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
  Vector3 enemyDeadPos_{};

  // 岩マネージャ
  std::unique_ptr<RockManager> rockManager_ = nullptr;

  // フィールド
  Field field_;

  // 天球
  std::unique_ptr<SkyDome> skyDome_;

  // フィールドフェード開始フラグ
  bool fieldFadeStarted_ = false;

  // パーティクル(HitEffect)
  std::unique_ptr<ParticleSystem> hitEffects_ = nullptr;
  // パーティクル(Explosion)
  std::unique_ptr<ParticleSystem> explosion_ = nullptr;

  //----------SE-------------
  Se playerAttackToEnemySE_;
  Se playerAttackToWallSE_;
  Se enemyAttackToPlayerSE_;
  Se playerDeadSE_;
  Se cursolSE_;
  Se decisionSE_;
  Se enemyDeadSE_;
  Bgm inGameBGM_;

  //-----UI-----
  std::unique_ptr<EnemyHpGauge> enemyHpGauge_;

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

  // 遷移フェード
  Fade fade_;
  std::string nextSceneName_;

  RockMulti rockMulti_;

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

private:
  GameState state = GameState::Tutorial;

  float playerDeadTimer_ = 0.0f;
  float playerDeadDuration_ = 2.0f;

  float enemyDeadTimer_ = 0.0f;
  float enemyDeadDuration_ = 2.0f;

  Vector3 deadCamStartPos_{};  // 演出開始時のカメラ位置
  Vector3 deadCamTargetPos_{}; // 目標位置
  float deadCamStartFov_ = 0.0f;
  float deadCamTargetFov_ = 0.0f;

  float worldFade_ = 0.0f;      // 0.0＝通常、1.0＝真っ暗
  float worldFadeSpeed_ = 1.0f; // 1秒で暗くなる値

  // --- GameOver UI ---
  Sprite gameOverSprite_;
  Sprite retrySprite_;
  Sprite titleSprite_;

  // GAME OVER スタンプ演出用
  bool  gameOverStampPlaying_ = false;
  float gameOverStampTimer_ = 0.0f;
  float gameOverStampDuration_ = 0.8f;   // 落ちてくる時間（秒）
  Vector3 gameOverBasePos_{ 640.0f, 220.0f, 0.0f };

  // --- GameClear UI ---
  Sprite gameClearSprite_;

  // GAME CLEAR スタンプ演出用
  bool  gameClearStampPlaying_ = false;
  float gameClearStampTimer_ = 0.0f;
  float gameClearStampDuration_ = 0.8f;   // 同じくらいの速度
  Vector3 gameClearBasePos_{ 640.0f, 220.0f, 0.0f };
  // GameOver 選択肢の点滅用
  float gameOverBlinkTimer_ = 0.0f;
  bool  gameOverBlinkOn_ = true;

public:
  Vector4 GetWorldDarkColor() const {
    float k = 1.0f - worldFade_;
    return Vector4{k, k, k, 1.0f};
  }

private:
  int resultIndex_ = 0; // リザルト時の選択肢　1:リトライ、2:タイトル

  int prevRockMultiplier_ = 1;

  TutorialState tutorialState_ = TutorialState::Rock;
  Sprite tutorialRSptite_;
  bool tutorialHitEnemy_ = false; // 敵に当てたか
  bool tutorialDamaged_ = false;  // ダメージを受けたか
  bool tutorialAttackDone_ = false;

  // フェーズ2移行時のカメラ演出が開始済みかどうか
  bool phase2CameraEffectStarted_ = false;

public:
  static bool s_hasPlayedTutorial_;
};
