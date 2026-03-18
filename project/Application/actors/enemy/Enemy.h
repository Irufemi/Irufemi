#pragma once
#include "EnemyAI.h"
#include "EnemyAnimation.h"
#include "EnemyBeam.h"
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

// 敵の行動状態
enum class EnemyState {
    Idle,        // 待機：ふわふわ浮遊しながら自転
    Attack_Beam, // 攻撃：集結・シェイク後のビーム発射演出
    Attack_Missile, // 攻撃：ミサイル
    Damaged      // 被弾：ノックバック演出など
};

class Enemy {
public:
    ~Enemy();
    void Initialize(Camera* camera, IrufemiEngine* engine = nullptr);
    void Update(Player* player);
    void Draw(IrufemiEngine* engine);

    // --- ビーム制御用 ---
    void FireBeam(); // ビームを生成する関数
    bool IsFiringBeam() const { return beam_ != nullptr; }
    bool IsFiringRealBeam() const { return animation_ != nullptr && animation_->IsFiring(); }
    EnemyBeam* GetBeam() const { return beam_.get(); }

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

    void SetState(EnemyState state) { state_ = state; }
    EnemyState GetState() const { return state_; }

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

private:
    // 各パーツ
    std::array<std::unique_ptr<Body>, 3> bodies_;
    std::unique_ptr<HeadLeft> headLeft_ = nullptr;
    std::unique_ptr<HeadMid> headMid_ = nullptr;
    std::unique_ptr<HeadRight> headRight_ = nullptr;

    // ビームのインスタンス管理
    std::unique_ptr<EnemyBeam> beam_ = nullptr;

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
    Camera* camera_ = nullptr;

    // パラメータ
    float fallSpeed_ = 0.01f;     // 落下時の補間速度
    float shakeIntensity_ = 1.0f; // 着地時のシェイク強度
    bool isFalling_[4] = { false, false, false, false };

#ifdef USE_IMGUI
    bool isDebugDrawOBB_ = false;
    std::unique_ptr<Line3DRegion> lineOBB_ = nullptr;
    IrufemiEngine* engine_ = nullptr;
#endif

  bool isActive_ = false;
  bool isDead_ = false;

};
