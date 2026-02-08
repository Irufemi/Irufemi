#pragma once

#include "math/Vector3.h"
#include "math/Transform.h"
#include "math/LRDirection.h"
#include "contents/MapChipField.h"

class Player;
class InputManager;

class PlayerPhysics {
public:
    struct CollisionMapInfo {
        bool isContactCeiling = false;
        bool isContactGround = false;
        bool isContactWall = false;
        int  wallDir = 0;
        Vector3 amountMove{};
    };

    void Initialize(Player* player, Transform* transform, MapChipField* mapChipField, InputManager* inputManager);
    void Update();
    void MoveInput();
    void ApplyGravity();
    void BehaviorMoveUpdate();

    // ゲッター
    const Vector3& GetVelocity() const { return velocity_; }
    bool IsOnGround() const { return onGround_; }
    bool IsTouchingWall() const { return isTouchingWall_; }
    int GetLastWallDir() const { return lastWallDir_; }
    int GetWallCoyoteCounter() const { return wallCoyoteCounter_; }
    int GetAirJumpsLeft() const { return airJumpsLeft_; }
    bool IsDashUsed() const { return dashUsed_; }

    // セッター
    void SetVelocity(const Vector3& vel) { velocity_ = vel; }
    void SetAirJumpsLeft(int count) { airJumpsLeft_ = count; }
    void SetDashUsed(bool used) { dashUsed_ = used; }
    void ResetHorizontalLock() { horizontalControlLockTimer_ = 0.0f; }


private:
    void TurningControl();
    void CollisionDetection(CollisionMapInfo& info);
    void MoveAccordingly(const CollisionMapInfo& info);
    void ContactCeiling(const CollisionMapInfo& info);
    void ContactGround(const CollisionMapInfo& info);
    void ContactWall(const CollisionMapInfo& info);
    bool IsSolidAt(const Vector3& p, MapChipField::IndexSet* outIdx, MapChipField::Rect* outRect) const;
    float ResolveVerticalFrom(const Vector3& base, float dy, CollisionMapInfo& info) const;
    float ResolveHorizontalFrom(const Vector3& base, float dx, CollisionMapInfo& info) const;

private:
    Player* player_ = nullptr;
    Transform* transform_ = nullptr;
    MapChipField* mapChipField_ = nullptr;
    InputManager* inputManager_ = nullptr;

    Vector3 velocity_{};
    bool onGround_ = true;
    int coyoteCounter_ = 0;
    int jumpBufferCounter_ = 0;
    int airJumpsLeft_ = 0;
    bool jumpHeldPrev_ = false;
    bool isTouchingWall_ = false;
    int lastWallDir_ = 0;
    int wallCoyoteCounter_ = 0;
    float horizontalControlLockTimer_ = 0.0f;
    bool dashUsed_ = false;

    // 旋回用
    float turnFirstRotationY_ = 0.0f;
    float turnTimer_ = 0.0f;
};