#pragma once
#include "EnemyState.h"
#include "AI/EnemyAI.h"
#include "Animation/EnemyAnimation.h"
#include "Beam/EnemyBeam.h"
#include "StompEffects/EnemyStompEffects.h"
#include "TackleEffects/EnemyTackleEffects.h"
#include "Bomb/EnemyBomb.h"
#include "Body/Body.h"
#include "Head/Left/HeadLeft.h"
#include "Head/Mid/HeadMid.h"
#include "Head/Right/HeadRight.h"
#include "core/math/Transform.h"
#include "core/math/geometry/OBB.h"
#include <array>
#include <memory>
#include <vector>

class Camera;
class IrufemiEngine;
class Line3DRegion;
class EnemyHPBar;
class EnemyPartHPBar;
class WeaponTrail;
class Sprite;

class Enemy {
public:
    Enemy();
    ~Enemy();
    void Initialize(IrufemiEngine* engine = nullptr);
    void Update(Player* player);
    void Draw(IrufemiEngine* engine);
    
    // UI関連手続き
    void Draw3DUI(IrufemiEngine* engine, bool isUI = false);
    void Draw2DUI(IrufemiEngine* engine, bool isFirstPerson = true);

    // --- ビーム制御用 ---
    void FireBeam(); // ビームを生成する関数
    bool IsFiringBeam(int index) const { return beams_[index] != nullptr; }
    bool IsFiringRealBeam() const { return animation_ != nullptr && animation_->IsFiring(); }
    EnemyBeam* GetBeam(int index) const { return beams_[index].get(); }
    
    // --- 爆弾制御用 ---
    void FireBomb(int index, const Vector3& targetPos);
    EnemyBomb* GetBomb(int index) const { return bombs_[index].get(); }
    
    // --- スタンプ制御用 ---
    void FireStomp(const Vector3& position);
    EnemyStompEffects* GetStompEffects() const { return stompEffects_.get(); }

    // --- タックルエフェクト用 ---
    void FireTackleRushWave(const Vector3& position);
    void FireTackleCrashWave(const Vector3& position);
    EnemyTackleEffects* GetTackleEffects() const { return tackleEffects_.get(); }

    // --- トレイルエフェクト用 ---
    WeaponTrail* GetNeckTrail() const { return neckTrail_.get(); }

    // --- アクセサ（AIやAnimationから操作用） ---
    Transform& GetGlobalTransform() { return globalTransform_; }
    Transform& GetBodyLocalTransform(int index) {
        return bodyLocalTransforms_[index];
    }
    Transform& GetHeadLeftLocalTransform() { return headLeftLocalTransform_; }
    Transform& GetHeadMidLocalTransform() { return headMidLocalTransform_; }
    Transform& GetHeadRightLocalTransform() { return headRightLocalTransform_; }

    Vector3& GetBodyOffset(int index) { return bodyOffsets_[index]; }
    Vector3& GetHeadLeftOffset() { return headLeftOffset_; }
    Vector3& GetHeadMidOffset() { return headMidOffset_; }
    Vector3& GetHeadRightOffset() { return headRightOffset_; }

    void SetState(EnemyState newState);
    EnemyState GetState() const { return state_; }

    bool GetIsPhase2() const { return isPhase2_; }
    void SetIsPhase2(bool isPhase2) { isPhase2_ = isPhase2; }

    Vector3 GetTargetPosition() const;

    // サンドバッグモード設定（AI無効化、死亡・フェーズ2移行の防止）
    void SetIsSandbagMode(bool isSandbag) { isSandbagMode_ = isSandbag; }
    bool GetIsSandbagMode() const { return isSandbagMode_; }

    // 警告演出アクセス用
    void SetWarningActive(bool active) { isWarningActive_ = active; }
    bool IsWarningActive() const { return isWarningActive_; }

    // アニメーションクラスへのアクセス用
    EnemyAnimation* GetAnimation() const { return animation_.get(); }

    Matrix4x4 GetHeadMidWorldMatrix() const;
    OBB GetOBB() const; // 全体のOBB

    // --- 部位ごとの操作インターフェース ---
    Body* GetBody(int index) { return bodies_[index].get(); }
    HeadLeft* GetHeadLeft() { return headLeft_.get(); }
    HeadMid* GetHeadMid() { return headMid_.get(); }
    HeadRight* GetHeadRight() { return headRight_.get(); }

  bool GetIsActive() const { return isActive_; }
  bool IsDead() const { return isDead_; }
  bool IsHeadDead(int index) const;
  
  bool IsReadyForClearTransition() const;

private:
    // 各パーツ
    std::array<std::unique_ptr<Body>, 3> bodies_;
    std::unique_ptr<HeadLeft> headLeft_ = nullptr;
    std::unique_ptr<HeadMid> headMid_ = nullptr;
    std::unique_ptr<HeadRight> headRight_ = nullptr;

    // ビームのインスタンス管理
    std::array<std::unique_ptr<EnemyBeam>, 3> beams_;

    // 爆弾のインスタンス管理
    std::array<std::unique_ptr<EnemyBomb>, 3> bombs_;

    // スタンプのインスタンス管理
    std::unique_ptr<EnemyStompEffects> stompEffects_;
    std::unique_ptr<EnemyTackleEffects> tackleEffects_;

    // 撃破演出用の特大爆発エフェクト
    std::unique_ptr<GPUParticleSystem> deathExplosionParticle_;

    // 斬撃トレイル
    std::unique_ptr<WeaponTrail> neckTrail_ = nullptr;

    // トランスフォーム
    Transform globalTransform_;
    std::array<Transform, 3> bodyLocalTransforms_;
    Transform headLeftLocalTransform_;
    Transform headMidLocalTransform_;
    Transform headRightLocalTransform_;

    // アニメーション用オフセット
    std::array<Vector3, 3> bodyOffsets_ = {};
    Vector3 headLeftOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 headMidOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 headRightOffset_ = { 0.0f, 0.0f, 0.0f };

    // 制御クラス
    std::unique_ptr<EnemyAI> ai_ = nullptr;
    std::unique_ptr<EnemyAnimation> animation_ = nullptr;

    EnemyState state_ = EnemyState::Idle;
    bool isPhase2_ = false;


    // パラメータ
    float fallSpeed_ = 0.01f;     // 落下時の補間速度
    float shakeIntensity_ = 1.0f; // 着地時のシェイク強度
    bool isFalling_[4] = { false, false, false, false };

#ifdef USE_IMGUI
    void UpdateDebugUI();
    bool isDebugDrawOBB_ = false;
    std::unique_ptr<Line3DRegion> lineOBB_ = nullptr;
#endif
    IrufemiEngine* engine_ = nullptr;

  bool isActive_ = false;
  bool isDead_ = false;

  enum class DeathPhase { None, Reassembling, Gathered, Exploding, Aftermath };
  DeathPhase deathPhase_ = DeathPhase::None;
  float deathTimer_ = 0.0f;
  float aftermathExplosionTimer_ = 0.0f;

  // --- 死亡演出用調整定数（メンバー変数） ---
  static constexpr float kReassembleDuration = 4.5f;  ///< バラバラになった部位が中央で合体するまでの時間（秒）
  static constexpr float kHoldDuration = 2.0f;        ///< 合体してからボクセル爆散するまでの激しくのたうち悶えるタメ時間（秒）
  static constexpr float kAftermathDuration = 10.0f;   ///< ホワイトアウト中も爆発し続けるための十分な長さ

  // 悶えのたうち（Agony）全体のサイン波パラメータ
  static constexpr float kAgonyPitchFreq = 12.0f;     ///< のたうち全体のPitch（X軸回転）振動周波数
  static constexpr float kAgonyPitchAmp = 0.18f;      ///< のたうち全体のPitch（X軸回転）最大振幅
  static constexpr float kAgonyRollFreq = 15.0f;      ///< のたうち全体のRoll（Z軸回転）振動周波数
  static constexpr float kAgonyRollAmp = 0.22f;       ///< のたうち全体のRoll（Z軸回転）最大振幅

  // 小刻みな高速振動（ブルブル感）と上下の激しいのたうちパラメータ
  static constexpr float kShakeFreq = 70.0f;          ///< タメ中の小刻みなブルブル高速振動周波数
  static constexpr float kShakeAmp = 0.12f;           ///< タメ中の小刻みなブルブル高速振動の幅
  static constexpr float kVerticalFreq = 18.0f;       ///< タメ中のダイナミックな上下のたうち周波数
  static constexpr float kVerticalAmp = 0.6f;         ///< タメ中のダイナミックな上下のたうち振幅

  // 頭部（首）のうねる動きパラメータ
  static constexpr float kHeadWiggleFreq = 20.0f;     ///< 3頭部が悶えてうねる振動周波数
  static constexpr float kHeadWiggleAmp = 1.5f;       ///< 3頭部が悶えてうねる最大振幅

  // 爆散はじけ飛びパラメータ
  static constexpr float kExplosionBlowSpeed = 2.2f;  ///< 爆散時に各パーツが放射状にはじけ飛ぶ初速

  std::array<Transform, 3> initialBodyLocalTransforms_;
  Transform initialHeadLeftLocalTransform_ = {};
  Transform initialHeadMidLocalTransform_ = {};
  Transform initialHeadRightLocalTransform_ = {};

  Transform startBodyLocalTransforms_[3] = {};
  Transform startHeadLeftLocalTransform_ = {};
  Transform startHeadMidLocalTransform_ = {};
  Transform startHeadRightLocalTransform_ = {};
  Vector3 startGlobalTranslate_ = {};

  void UpdateDeathPhase(float deltaTime, Player* player = nullptr);

    // UI
    std::unique_ptr<EnemyHPBar> hpBar_ = nullptr;
    std::vector<std::unique_ptr<EnemyPartHPBar>> partHPBars_;

    bool isSandbagMode_ = false;

    // 警告演出用
    std::unique_ptr<Sprite> warningSprite_ = nullptr;
    std::unique_ptr<Sprite> warningArrowLeft1_ = nullptr;
    std::unique_ptr<Sprite> warningArrowLeft2_ = nullptr;
    std::unique_ptr<Sprite> warningArrowRight1_ = nullptr;
    std::unique_ptr<Sprite> warningArrowRight2_ = nullptr;
    bool isWarningActive_ = false;
    float warningTimer_ = 0.0f;

    // 自機死亡時の強制キャンセル制御用フラグ
    bool wasPlayerDeathHandled_ = false;

    // ビネット調整用パラメータ (ImGuiでリアルタイム調整可能)
    float vignetteBaseScale_ = 40.0f;      // 基準クリア領域 (大きいほど赤が薄くなる)
    float vignetteScalePulseWidth_ = 2.0f; // 脈動の振幅
    float vignetteBasePower_ = 0.15f;       // 基準にじみ具合 (小さいほど赤が薄くなる)
    float vignettePowerPulseWidth_ = 0.05f; // 脈動の振幅
};
