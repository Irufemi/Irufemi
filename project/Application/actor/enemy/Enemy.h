#pragma once
#include "3D/ObjClass.h"
#include "EnemyBullet.h"
#include "EnemyWall.h"
#include "camera/Camera.h"
#include"audio/Se.h"

class Player;

// プレイヤーとの当たり判定結果をまとめる構造体
struct EnemyPlayerHitResult {
  // 壁に当たったか
  bool hitWall = false;
  int wallIndex = -1;

  // 弾に当たったか
  bool hitBullet = false;
  int bulletIndex = -1;

  // 敵本体に当たったか
  bool hitEnemyBody = false;
};

// 敵の行動状態
enum class EnemyState {
  Idle = 0,      // 何もしていない（次の行動待ち）
  WallRise,      // ドッスンのように上昇中
  WallDrop,      // ドッスン落下中（落ちた瞬間に壁生成）
  BulletCharge,  // 弾のチャージ中（赤くなっていくイメージ）
  BurrowPreDive, // 潜る前のもぞもぞ
  BurrowHidden,  // 潜って見えない状態（地中移動中）
  BurrowEmerge,  // 出現時のもぞもぞ
  ChargeBack,    // 突進の予備動作（後ろに下がる）
  DashForward,   // 高速突進中
  Dead           // フェーズ2でHP0になった後のやられ演出
};

// フェーズ
enum class EnemyPhase {
  Phase1 = 0,
  Phase2 = 1,
};

class Enemy {
public:
  /// <summary>
  /// 初期化
  /// </summary>
  Enemy();

  /// <summary>
  /// 壁マネージャと弾マネージャのポインタを渡す
  /// </summary>
  /// <param name="camera">カメラ</param>
  /// <param name="spawnPos">初期位置</param>
  /// <param name="stageRadius">ステージ半径</param>
  /// <param name="wallManager">壁マネージャー</param>
  /// <param name="bulletManager">弾マネージャー</param>
  void Initialize(Camera *camera, const Vector3 &spawnPos,
                  const float &stageRadius, EnemyWallManager *wallManager,
                  EnemyBulletManager *bulletManager);

  /// <summary>
  /// 更新処理
  /// </summary>
  /// <param name="deltaTime">時間</param>
  /// <param name="playerPos">プレイヤー座標</param>
  void Update(float deltaTime, const Vector3 &playerPos);

  /// <summary>
  /// 描画処理
  /// </summary>
  void Draw();

  /// <summary>
  /// 現在位置のゲッター
  /// </summary>
  /// <returns>敵の座標</returns>
  const Vector3 &GetPosition() const { return transform_.translate; }

  /// <summary>
  /// 現在位置のセッター
  /// </summary>
  /// <returns>敵の半径</returns>
  const float &GetRadius() const { return enemyBodyRadius_; }

  // -------------------------------
  // プレイヤーとの当たり判定関連
  // -------------------------------

  /// <summary>
  /// 直近フレームのプレイヤーとの当たり判定結果を取得
  /// </summary>
  /// <returns>直近フレームのプレイヤーとの当たり判定結果</returns>
  const EnemyPlayerHitResult &GetPlayerHitResult() const {
    return lastHitResult_;
  }

  // 手動で結果をクリアしたい場合はこれを呼ぶ
  void ClearPlayerHitResult() { lastHitResult_ = EnemyPlayerHitResult{}; }

  // -------------------------------
  // プレイヤーからの攻撃関連
  // -------------------------------

  // プレイヤーの攻撃力などに応じてダメージを与える
  void ApplyDamageFromPlayer(int damage);

  int GetHp() const { return hp_; }

  // フェーズ2かつHP0以下で死亡扱い
  bool IsDead() const { return (phase_ == EnemyPhase::Phase2) && (hp_ <= 0); }

  // -------------------------------
  // 予備動作用の情報（見た目用に他クラスから参照したくなったとき用）
  // -------------------------------

  // 現在の行動状態（デバッグ表示などに使える）
  EnemyState GetState() const { return state_; }

  // 弾チャージの進み具合（0.0〜1.0）
  float GetBulletChargeProgress() const { return bulletChargeProgress_; }

  // 現在のフェーズ
  EnemyPhase GetPhase() const { return phase_; }

  // 突進を強制停止（プレイヤーに当たったときなどに呼ぶ）
  void ForceStopDash();

  // スタン状態を開始（durationFrame フレーム間スタンする）
  void StartStan(int durationFrame) {
    isStan_ = true;
    stanTimer_ = durationFrame;
  }

private:
  Camera *camera_ = nullptr;
  std::unique_ptr<ObjClass> model_ = nullptr;

  struct TransformLocal {
    Vector3 translate{0.0f, 0.0f, 0.0f};
    Vector3 rotate{0.0f, 0.0f, 0.0f};
    Vector3 scale{1.0f, 1.0f, 1.0f};
  } transform_;

  // HP
  int maxHp_ = 140;
  int hp_ = 140;
  EnemyPhase phase_ = EnemyPhase::Phase1;

  EnemyWallManager *enemyWall_ = nullptr;
  EnemyBulletManager *enemyBullet_ = nullptr;

  // 行動タイマー（Idle のときだけ使う：次に行動を始めるまでの時間）
  float actionTimer_ = 0.0f;
  float actionIntervalMin_ = 2.0f; // フェーズ1：2〜4秒の間で行動開始
  float actionIntervalMax_ = 4.0f;

  // 潜り移動用（isBurrowing_ は「見えていない間」だけ true にする）
  bool isBurrowing_ = false; // 見えていない間だけ true
  float burrowTimer_ = 0.0f; // 旧処理の名残だが保持しておく
  float burrowDuration_ = 1.0f;

  Vector3 stageCenter_{0.0f, 0.0f, 0.0f}; // ステージ中心（仮に原点）
  float stageRadius_{};                   // ステージ半径

  // 敵本体の当たり判定用半径（円として扱う）
  float enemyBodyRadius_ = 1.5f;

  // 敵モデルの下端が地面にくるように持ち上げる量
  float enemyHeightOffset_ = 1.5f;

  // プレイヤーとの直近フレームの当たり判定結果
  EnemyPlayerHitResult lastHitResult_;

  // --------------------------------
  // 予備動作 + 行動用の状態マシン
  // --------------------------------
  EnemyState state_ = EnemyState::Idle; // 現在の行動状態
  float stateTimer_ = 0.0f;             // 現在の状態の残り時間（秒）

  // 地面の高さ（ドッスン上下やもぞもぞの基準）
  float groundY_ = 0.0f;

  // 壁攻撃（ドッスン）用パラメータ
  float wallRiseHeight_ = 4.0f;   // どれくらい持ち上がるか
  float wallRiseDuration_ = 0.4f; // 上昇時間
  float wallDropDuration_ = 0.2f; // 落下時間

  // 弾攻撃のチャージ時間
  float bulletChargeDuration_ = 0.8f;
  float bulletChargeProgress_ = 0.0f; // 0.0〜1.0

  // 弾の見た目用（膨張・収縮）
  Vector3 bulletChargeBaseScale_{1.0f, 1.0f, 1.0f};
  float bulletChargeBaseHeight_ = 0.0f;
  float bulletChargeScaleAmount_ = 0.25f; // Max で 25% くらい大きくする

  // フェーズ2用の弾拡散角度（1 発ごとの間隔）
  float bulletSpreadAngleRad_ = 0.20f; // おおよそ 11.5 度

  // 潜り攻撃用パラメータ
  float burrowPreDuration_ = 0.8f;      // 潜る前のもぞもぞ時間
  float burrowHiddenDuration_ = 1.0f;   // 地中にいる時間
  float burrowEmergeDuration_ = 0.8f;   // 出現時のもぞもぞ時間
  float burrowWobbleAmplitude_ = 0.3f;  // もぞもぞの揺れの大きさ
  float burrowWobbleFrequency_ = 10.0f; // もぞもぞの揺れの速さ
  float burrowPhase_ = 0.0f;

  // --------- 突進攻撃用パラメータ ---------
  float dashChargeBackTime_ = 0.4f; // 予備動作の時間
  float dashBackDistance_ = 1.5f;   // 後ろに下がる距離

  float dashSpeed_ = 18.0f;    // 突進の速度
  float dashDistance_ = 10.0f; // 走る距離
  float dashMoved_ = 0.0f;     // 現在走った距離

  Vector3 dashDirection_{0, 0, 0}; // 向いている方向のキャッシュ

  // 突進で場外に出たあと、次の行動を確定で潜り移動にするフラグ
  bool forceBurrowOnNextAction_ = false;

  // --------- 行動オン/オフフラグ ---------
  bool enableWallAttack_ = true;
  bool enableBulletAttack_ = true;
  bool enableBurrowMove_ = true;
  bool enableDashAttack_ = true;

  // --------- 近/中/遠距離の行動重み ---------
  struct ActionWeightSet {
    float bullet = 0.0f;
    float wall = 0.0f;
    float burrow = 0.0f;
    float dash = 0.0f;
  };

  // 近距離: dist < 6.0f
  ActionWeightSet weightsNear_{10.0f, 10.0f, 30.0f, 50.0f};
  // 中距離: 6.0f <= dist < 12.0f（素の確率）
  ActionWeightSet weightsMid_{40.0f, 20.0f, 10.0f, 30.0f};
  // 遠距離: dist >= 12.0f
  ActionWeightSet weightsFar_{60.0f, 20.0f, 10.0f, 10.0f};

  //----- やられ中パラメーター -----
  bool isStan_ = false; // 無敵か
  int stanTimer_ = 0;   // 無敵フレーム数
  int invincibleBlinkCounter_ = 0;
  Vector4 normalColor_{1.0f, 1.0f, 1.0f, 1.0f};
  Vector4 stanColor_{5.0f, 5.0f, 5.0f, 1.0f};

    // 敵死亡演出用
  bool deathStarted_ = false;        // 死亡演出が始まったか
  float deathTimer_ = 0.0f;          // 死亡演出の経過時間
  float deathDuration_ = 1.5f;       // 完全に小さくなるまでの時間（秒）
  float deathRotateSpeedY_ = 360.0f; // 1秒あたりのY回転量（度）
  Vector3 deathStartScale_{1.0f, 1.0f, 1.0f}; // 演出開始時のスケール

    // ------- フェーズ2移行演出用 -------
  bool phase2TransitionActive_ = false;   // フェーズ2演出中か
  float phase2TransitionTimer_ = 0.0f;    // 経過時間（秒）
  float phase2TransitionDuration_ = 3.0f; // 演出時間（秒）
  // 吠えるときの「少し上を向く」角度（ラジアン）
  float phase2RoarAngleRad_ =
      -20.0f * 3.141592654f / 180.0f; // 上方向に20度くらい

  void ResetActionTimer();
  Vector3 GetRandomReappearPosition(const Vector3 &playerPos) const;
  bool IsInsideAnyWall(const Vector3 &pos, float margin) const;

  // フェーズ2突入処理
  void EnterPhase2();

  // XZ平面でのベクトル長さを返す
  float LengthXZ(const Vector3 &v);

  // 敵位置 origin から、target（プレイヤー）方向を向かせる
  void DirectionFacing(const Vector3 &origin,const Vector3 &target);

  //SEの初期化
  Se enemyBulletSE_;
  Se enemyWallSE_;
  Se enemyDashSE_;
  Se enemyChangeSE_;
  Se enemyBurrowSE_;    
public:
  /// <summary>
  /// 潜っているかどうか
  /// </summary>
  /// <returns></returns>
  bool IsBurrowing() const { return isBurrowing_; }
};
