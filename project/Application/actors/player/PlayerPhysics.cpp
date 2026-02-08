#define NOMINMAX
#include "PlayerPhysics.h"
#include "Player.h"
#include "engine/Input/InputManager.h"
#include "function/Math.h"
#include "function/Ease.h"
#include <algorithm>
#include <numbers>
#include <cmath>

void PlayerPhysics::Initialize(Player* player, Transform* transform, MapChipField* mapChipField, InputManager* inputManager) {
    player_ = player;
    transform_ = transform;
    mapChipField_ = mapChipField;
    inputManager_ = inputManager;

    velocity_ = {};
    onGround_ = true;
    airJumpsLeft_ = Player::kMaxAirJumps;
    jumpHeldPrev_ = false;
    dashUsed_ = false;
}

void PlayerPhysics::Update() {
    // PlayerクラスのUpdateから呼び出される
}

void PlayerPhysics::MoveInput() {
    const bool keyRight = inputManager_->IsKeyDown('D');
    const bool keyLeft = inputManager_->IsKeyDown('A');
    const bool keyJump = inputManager_->IsKeyDown('W');
    const bool padRight = inputManager_->DPadRight() || inputManager_->GetLeftStickX() > 0.5f;
    const bool padLeft = inputManager_->DPadLeft() || inputManager_->GetLeftStickX() < -0.5f;
    const bool padJump = inputManager_->IsButtonDown(XINPUT_GAMEPAD_A);
    const bool right = keyRight || padRight;
    const bool left = keyLeft || padLeft;
    const bool jumpDown = keyJump || padJump;
    const bool jumpTriggered = jumpDown && !jumpHeldPrev_;
    const float dt = 1.0f / 60.0f;

    if (jumpDown) jumpBufferCounter_ = Player::kJumpBufferFrames;
    else if (jumpBufferCounter_ > 0) --jumpBufferCounter_;

    if (horizontalControlLockTimer_ > 0.0f) {
        horizontalControlLockTimer_ = std::max(0.0f, horizontalControlLockTimer_ - dt);
    }

    int inputX = (right ? 1 : 0) - (left ? 1 : 0);
    if (inputX != 0) {
        LRDirection newDir = (inputX > 0) ? LRDirection::kRight : LRDirection::kLeft;
        if (player_->GetLR() != newDir) {
            player_->SetLRDirection(newDir);
            turnFirstRotationY_ = transform_->rotate.y;
            turnTimer_ = Player::kTimeTurn;
        }
    }

    const float targetX = static_cast<float>(inputX) * Player::kLimitRunSpeed;
    if (horizontalControlLockTimer_ <= 0.0f) {
        const float alpha = std::clamp(dt / Player::kTimeToFullRun, 0.0f, 1.0f);
        velocity_.x += (targetX - velocity_.x) * alpha;
        velocity_.x = std::clamp(velocity_.x, -Player::kLimitRunSpeed, Player::kLimitRunSpeed);
    } else {
        velocity_.x = std::clamp(velocity_.x, -Player::kLimitRunSpeed, Player::kLimitRunSpeed);
    }

    if ((onGround_ || coyoteCounter_ > 0) && jumpTriggered) {
        velocity_.y = Player::kJumpAcceleration;
        onGround_ = false;
        coyoteCounter_ = 0;
        jumpBufferCounter_ = 0;
    } else if (!onGround_ && jumpTriggered && (isTouchingWall_ || wallCoyoteCounter_ > 0)) {
        const int dir = (lastWallDir_ == 0) ? ((right && !left) ? +1 : (left && !right) ? -1 : +1) : lastWallDir_;
        velocity_.y = Player::kWallJumpVertical;
        velocity_.x = (dir > 0) ? -Player::kWallJumpHorizontal : +Player::kWallJumpHorizontal;
        LRDirection newDir = (dir > 0) ? LRDirection::kLeft : LRDirection::kRight;
        if (player_->GetLR() != newDir) {
            player_->SetLRDirection(newDir);
            turnFirstRotationY_ = transform_->rotate.y;
            turnTimer_ = Player::kTimeTurn;
        }
        airJumpsLeft_ = Player::kMaxAirJumps;
        horizontalControlLockTimer_ = Player::kWallJumpHorizLockTime;
        dashUsed_ = false;
        onGround_ = false;
        coyoteCounter_ = 0;
        jumpBufferCounter_ = 0;
        isTouchingWall_ = false;
        wallCoyoteCounter_ = 0;
    } else if (!onGround_ && jumpTriggered && airJumpsLeft_ > 0) {
        velocity_.y = Player::kJumpAcceleration;
        --airJumpsLeft_;
        coyoteCounter_ = 0;
        jumpBufferCounter_ = 0;
    }

    const bool jumpHeld = jumpDown;
    if (!jumpHeld && velocity_.y > 0.0f) {
        velocity_.y *= Player::kJumpCutFactor;
    }

    ApplyGravity();
    jumpHeldPrev_ = jumpDown;
}

void PlayerPhysics::ApplyGravity() {
    if (!onGround_) {
        float g = (velocity_.y <= 0.0f) ? Player::kgravityAcceleration * Player::kFallGravityScale : Player::kgravityAcceleration;
        velocity_ = Math::Add(velocity_, Vector3(0.0f, -g, 0.0f));
        velocity_.y = std::max(velocity_.y, -Player::kLimitFallSpeed);
    }
}

void PlayerPhysics::BehaviorMoveUpdate() {
    TurningControl();
    CollisionMapInfo info{};
    info.amountMove = velocity_;
    CollisionDetection(info);
    MoveAccordingly(info);
    ContactCeiling(info);
    ContactGround(info);
    ContactWall(info);
}

void PlayerPhysics::TurningControl() {
    if (turnTimer_ <= 0.0f) {
        transform_->rotate.y = (player_->GetLR() == LRDirection::kRight) ? std::numbers::pi_v<float> / 2.0f : -std::numbers::pi_v<float> / 2.0f;
        return;
    }
    const float dt = 1.0f / 60.0f;
    float t = std::clamp(1.0f - (turnTimer_ / Player::kTimeTurn), 0.0f, 1.0f);
    float target = (player_->GetLR() == LRDirection::kRight) ? std::numbers::pi_v<float> / 2.0f : -std::numbers::pi_v<float> / 2.0f;
    transform_->rotate.y = Lerp(turnFirstRotationY_, target, EaseOutSine(t));
    turnTimer_ = std::max(0.0f, turnTimer_ - dt);
}

void PlayerPhysics::CollisionDetection(CollisionMapInfo& info) {
    Vector3 base = transform_->translate;
    float dy = ResolveVerticalFrom(base, info.amountMove.y, info);
    base.y += dy;
    float dx = ResolveHorizontalFrom(base, info.amountMove.x, info);
    info.amountMove = Vector3{ dx, dy, 0.0f };
}

float PlayerPhysics::ResolveVerticalFrom(const Vector3& base, float dy, CollisionMapInfo& info) const {
    if (dy == 0.0f) { return 0.0f; }
    const float hx = Player::kWidth * 0.5f;
    const float hy = Player::kHeight * 0.5f;
    float allowed = dy;
    if (dy > 0.0f) {
        const float topNew = base.y + dy + hy;
        Vector3 pL{ base.x - hx, topNew, 0.0f };
        Vector3 pR{ base.x + hx, topNew, 0.0f };
        MapChipField::IndexSet idx;
        MapChipField::Rect r;
        if (IsSolidAt(pL, &idx, &r) || IsSolidAt(pR, &idx, &r)) {
            float cand = (r.bottom - Player::kMBlank) - (base.y + hy);
            allowed = std::min(allowed, cand);
            info.isContactCeiling = true;
        }
    } else {
        const float botNew = base.y + dy - hy;
        Vector3 pL{ base.x - hx, botNew, 0.0f };
        Vector3 pR{ base.x + hx, botNew, 0.0f };
        MapChipField::IndexSet idx;
        MapChipField::Rect r;
        if (IsSolidAt(pL, &idx, &r) || IsSolidAt(pR, &idx, &r)) {
            float cand = (r.top + Player::kMBlank) - (base.y - hy);
            allowed = std::max(allowed, cand);
            info.isContactGround = true;
        }
    }
    return allowed;
}

float PlayerPhysics::ResolveHorizontalFrom(const Vector3& base, float dx, CollisionMapInfo& info) const {
    if (dx == 0.0f) { return 0.0f; }
    const float hx = Player::kWidth * 0.5f;
    const float hy = Player::kHeight * 0.5f;
    float allowed = dx;
    if (dx > 0.0f) {
        const float rightNew = base.x + dx + hx;
        Vector3 pT{ rightNew, base.y + hy, 0.0f };
        Vector3 pB{ rightNew, base.y - hy, 0.0f };
        MapChipField::IndexSet idx;
        MapChipField::Rect r;
        if (IsSolidAt(pT, &idx, &r) || IsSolidAt(pB, &idx, &r)) {
            float cand = (r.left - Player::kMBlank) - (base.x + hx);
            allowed = std::min(allowed, cand);
            info.isContactWall = true;
            info.wallDir = +1;
        }
    } else {
        const float leftNew = base.x + dx - hx;
        Vector3 pT{ leftNew, base.y + hy, 0.0f };
        Vector3 pB{ leftNew, base.y - hy, 0.0f };
        MapChipField::IndexSet idx;
        MapChipField::Rect r;
        if (IsSolidAt(pT, &idx, &r) || IsSolidAt(pB, &idx, &r)) {
            float cand = (r.right + Player::kMBlank) - (base.x - hx);
            allowed = std::max(allowed, cand);
            info.isContactWall = true;
            info.wallDir = -1;
        }
    }
    return allowed;
}

void PlayerPhysics::MoveAccordingly(const CollisionMapInfo& info) {
    transform_->translate = Math::Add(transform_->translate, info.amountMove);
}

bool PlayerPhysics::IsSolidAt(const Vector3& p, MapChipField::IndexSet* outIdx, MapChipField::Rect* outRect) const {
    auto idx = mapChipField_->GetMapChipIndexSetByPosition(p);
    if (outIdx) { *outIdx = idx; }
    MapChipType t = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
    if (t == MapChipType::kBlock) {
        if (outRect) { *outRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex); }
        return true;
    }
    return false;
}

void PlayerPhysics::ContactCeiling(const CollisionMapInfo& info) {
    if (info.isContactCeiling && velocity_.y > 0.0f) {
        velocity_.y = 0.0f;
    }
}

void PlayerPhysics::ContactGround(const CollisionMapInfo& info) {
    if (!onGround_ && info.isContactGround) {
        if (jumpBufferCounter_ > 0) {
            airJumpsLeft_ = Player::kMaxAirJumps;
            velocity_.y = Player::kJumpAcceleration;
            onGround_ = false;
            jumpBufferCounter_ = 0;
            coyoteCounter_ = 0;
            dashUsed_ = false;
            return;
        }
        onGround_ = true;
        dashUsed_ = false;
        velocity_.x *= (1.0f - Player::kAttenuationLanding);
        airJumpsLeft_ = Player::kMaxAirJumps;
    } else if (onGround_ && !info.isContactGround) {
        onGround_ = false;
    }

    if (info.isContactGround && onGround_) {
        coyoteCounter_ = Player::kCoyoteFrames;
    } else if (coyoteCounter_ > 0) {
        --coyoteCounter_;
    }
}

void PlayerPhysics::ContactWall(const CollisionMapInfo& info) {
    if (info.isContactWall) {
        velocity_.x *= (1.0f - Player::kAttenuationWall);
    }

    if (!onGround_ && info.isContactWall) {
        isTouchingWall_ = true;
        wallCoyoteCounter_ = Player::kWallCoyoteFrames;
        if (info.wallDir != 0) {
            lastWallDir_ = info.wallDir;
        }
    } else {
        isTouchingWall_ = false;
        if (wallCoyoteCounter_ > 0) {
            --wallCoyoteCounter_;
        } else {
            lastWallDir_ = 0;
        }
    }

    int wallDir = (info.wallDir != 0) ? info.wallDir : lastWallDir_;
    bool pressingToward = false;
    if (wallDir != 0 && inputManager_) {
        if (wallDir > 0) {
            pressingToward = inputManager_->IsKeyDown('D') || inputManager_->DPadRight() || inputManager_->GetLeftStickX() > 0.5f;
        } else {
            pressingToward = inputManager_->IsKeyDown('A') || inputManager_->DPadLeft() || inputManager_->GetLeftStickX() < -0.5f;
        }
    }
    if (!onGround_ && (isTouchingWall_ || info.isContactWall) && pressingToward && velocity_.y < 0.0f) {
        velocity_.y = std::max(velocity_.y, -Player::kWallSlideMaxFallSpeed);
    }
}