#pragma once
#include <memory>
#include <array>
#include <vector>
#include "actors/enemy/Body/Body.h"
#include "actors/enemy/Head/Left/HeadLeft.h"
#include "actors/enemy/Head/Mid/HeadMid.h"
#include "actors/enemy/Head/Right/HeadRight.h"
#include "EnemyAI.h"
#include "EnemyAnimation.h"
#include "math/Transform.h"

class Camera;

// 敵の行動状態
enum class EnemyState {
    Idle,      // 待機中
    Attack,    // 攻撃中
    Damaged    // 被弾時
};

class Enemy {
public:
    ~Enemy();
    void Initialize(Camera* camera);
    void Update();
    void Draw();

    // --- AIやAnimationから操作するためのゲッター ---
    Transform& GetGlobalTransform() { return globalTransform_; }

    Transform& GetBodyLocalTransform(int index) { return bodyLocalTransforms_[index]; }
    Transform& GetHeadLeftLocalTransform() { return headLeftLocalTransform_; }
    Transform& GetHeadMidLocalTransform() { return headMidLocalTransform_; }
    Transform& GetHeadRightLocalTransform() { return headRightLocalTransform_; }

    // アニメーション用オフセット座標へのアクセス
    Vector3& GetBodyOffset(int index) { return bodyOffsets_[index]; }
    Vector3& GetHeadLeftOffset() { return headLeftOffset_; }
    Vector3& GetHeadMidOffset() { return headMidOffset_; }
    Vector3& GetHeadRightOffset() { return headRightOffset_; }

    // 状態管理
    void SetState(EnemyState state) { state_ = state; }
    EnemyState GetState() const { return state_; }

private:
    std::array<std::unique_ptr<Body>, 3> bodies_;
    std::unique_ptr<HeadLeft> headLeft_ = nullptr;
    std::unique_ptr<HeadMid> headMid_ = nullptr;
    std::unique_ptr<HeadRight> headRight_ = nullptr;

    // 全体のTransform（親）
    Transform globalTransform_;

    // 各部位のローカルTransform（だるま落としの基準座標）
    std::array<Transform, 3> bodyLocalTransforms_;
    Transform headLeftLocalTransform_;
    Transform headMidLocalTransform_;
    Transform headRightLocalTransform_;

    // 各部位のアニメーション用オフセット
    std::array<Vector3, 3> bodyOffsets_;
    Vector3 headLeftOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 headMidOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 headRightOffset_ = { 0.0f, 0.0f, 0.0f };

    std::unique_ptr<EnemyAI> ai_ = nullptr;
    std::unique_ptr<EnemyAnimation> animation_ = nullptr;

    EnemyState state_ = EnemyState::Idle;
    Camera* camera_ = nullptr;

    float fallSpeed_ = 0.05f;
    float shakeIntensity_ = 1.0f;
    bool isFalling_[4] = { false, false, false, false };

    bool isActive_ = false;
};