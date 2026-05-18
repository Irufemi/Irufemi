#pragma once
#include "EnemyState.h"
#include "AI/EnemyAI.h"
#include "Animation/EnemyAnimation.h"
#include "Beam/EnemyBeam.h"
#include "StompEffects/EnemyStompEffects.h"
#include "TackleEffects/EnemyTackleEffects.h"
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

class Enemy {
public:
    ~Enemy();
    void Initialize(IrufemiEngine* engine = nullptr);
    void Update(Player* player);
    void Draw(IrufemiEngine* engine);
    
    // UI関連手続き
    void Draw3DUI(IrufemiEngine* engine, bool isUI = false);
    void Draw2DUI(IrufemiEngine* engine);

    // --- ビーム制御用 ---
    void FireBeam(); // ビームを生成する関数
    bool IsFiringBeam(int index) const { return beams_[index] != nullptr; }
    bool IsFiringRealBeam() const { return animation_ != nullptr && animation_->IsFiring(); }
    EnemyBeam* GetBeam(int index) const { return beams_[index].get(); }
    
    // --- スタンプ制御用 ---
    void FireStomp(const Vector3& position);
    EnemyStompEffects* GetStompEffects() const { return stompEffects_.get(); }

    // --- タックルエフェクト用 ---
    void FireTackleRushWave(const Vector3& position);
    void FireTackleCrashWave(const Vector3& position);
    EnemyTackleEffects* GetTackleEffects() const { return tackleEffects_.get(); }

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

private:
    // 各パーツ
    std::array<std::unique_ptr<Body>, 3> bodies_;
    std::unique_ptr<HeadLeft> headLeft_ = nullptr;
    std::unique_ptr<HeadMid> headMid_ = nullptr;
    std::unique_ptr<HeadRight> headRight_ = nullptr;

    // ビームのインスタンス管理
    std::array<std::unique_ptr<EnemyBeam>, 3> beams_;

    // スタンプのインスタンス管理
    std::unique_ptr<EnemyStompEffects> stompEffects_;
    std::unique_ptr<EnemyTackleEffects> tackleEffects_;

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

    // UI
    std::unique_ptr<EnemyHPBar> hpBar_ = nullptr;
    std::vector<std::unique_ptr<EnemyPartHPBar>> partHPBars_;

    bool isSandbagMode_ = false;
};
