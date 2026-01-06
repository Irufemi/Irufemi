#define NOMINMAX
#include "Player.h"

#include "function/Ease.h"
#include "contents/MapChipField.h"
#include "function/Math.h"
#include "engine/Input/InputManager.h"
#include "manager/DebugUI.h"
#include "PlayerState.h"
#include "3D/ObjClass.h"
#include "contents/enemy/IEnemy.h"
#include <algorithm>
#include <cassert>
#include <numbers>
#include <cmath>

// ===== ライフサイクル =====
void Player::Initialize(ObjClass* model, Camera* camera, InputManager* inputManager, Vector3& position) {
    assert(model);
    model_ = model;
    camera_ = camera;
    inputManager_ = inputManager;

    // Transform 初期化（右向きで開始）
    transform_.translate = position;
    transform_.rotate = Vector3{ 0.0f, std::numbers::pi_v<float> / 2.0f, 0.0f };
    transform_.scale = Vector3{ 1.0f, 1.0f, 1.0f };
    attackEffectTransform_ = transform_;

    // 追加初期化
    airJumpsLeft_ = kMaxAirJumps;
    jumpHeldPrev_ = false;
    dashUsed_ = false; // ダッシュ使用フラグ初期化
    hp_ = kMaxHP;
    invincibilityTimer_ = 0.0f;

    // 描画へ反映
    model_->SetTransform(transform_);

    // 初期状態は Root
    ChangeState(MakeRootState());

}

void Player::Update() {
#if defined USE_IMGUI
    ImGui::Begin("Player");
    ImGui::Text("State : %s", GetStateName());
    ImGui::Text("OnGround : %s", onGround_ ? "true" : "false");
    ImGui::Text("AirJumpsLeft : %d", airJumpsLeft_);
    ImGui::Text("TouchWall : %s (dir=%d, coyote=%d)", isTouchingWall_ ? "true" : "false", lastWallDir_, wallCoyoteCounter_);
    ImGui::Text("HP: %d/%d", hp_, kMaxHP);
    ImGui::Text("Invincibility: %.2f", invincibilityTimer_);
    ImGui::End();
#endif

    // ダメージフラグをリセット
    isJustDamaged_ = false;

    // 無敵時間処理
    if (invincibilityTimer_ > 0.0f) {
        const float dt = 1.0f / 60.0f;
        invincibilityTimer_ -= dt;

        // 点滅処理：周期を長くし、半透明にする
        float blinkCycle = 0.4f; // 点滅周期を0.4秒に
        float alpha = (std::fmod(invincibilityTimer_, blinkCycle) < blinkCycle / 2.0f) ? 0.3f : 1.0f; // 透明度を0.3に
        model_->SetAlpha(alpha);

        // 無敵時間に応じて赤から白へ色を戻す
        float colorLerpT = std::clamp(1.0f - (invincibilityTimer_ / kInvincibilityDuration), 0.0f, 1.0f);
        Vector4 red = { 1.0f, 0.5f, 0.5f, 1.0f };
        Vector4 white = { 1.0f, 1.0f, 1.0f, 1.0f };
        model_->SetColor(Lerp(red, white, colorLerpT));


        if (invincibilityTimer_ <= 0.0f) {
            // 無敵終了：色とアルファを元に戻す
            model_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
            model_->SetAlpha(1.0f);
        }
    }

    // ステート更新
    if (state_) {
        state_->Update(*this);
    }

    // 攻撃中でなければ移動と衝突判定を行う
    if (!IsAttacking()) {
        BehaviorMoveUpdate();
    } else {
        // 攻撃中でも旋回と行列更新は行う
        TurningControl();
        UpdateMatrix();
    }
}

void Player::Draw() {
    if (model_) {
        model_->SetTransform(transform_);
        model_->Update();
        model_->Draw();
    }
    if (IsAttacking() && attackEffectModel_) {
        attackEffectModel_->Update();
        attackEffectModel_->Draw();
    }
}

// ===== ステート制御 =====
void Player::ChangeState(std::unique_ptr<IPlayerState> next) {
    if (state_) {
        state_->Exit(*this);
    }
    state_ = std::move(next);
    if (state_) {
        state_->Enter(*this);
    }
}

/*
 * MoveInput
 * 入力一定速度（Hollow Knight 風）+ 二段/壁ジャン + 壁スライド
 */
void Player::MoveInput() {
    // 入力
    const bool right = inputManager_->IsKeyDown('D');
    const bool left = inputManager_->IsKeyDown('A');
    const bool jumpDown = inputManager_->IsKeyDown('W');
    const bool jumpTriggered = jumpDown && !jumpHeldPrev_;

    // 固定Δt（60fps前提）
    const float dt = 1.0f / 60.0f;

    // ジャンプバッファ
    if (jumpDown)
        jumpBufferCounter_ = kJumpBufferFrames;
    else if (jumpBufferCounter_ > 0)
        --jumpBufferCounter_;

    // 壁ジャン直後の横入力ロックの減衰
    if (horizontalControlLockTimer_ > 0.0f) {
        horizontalControlLockTimer_ = std::max(0.0f, horizontalControlLockTimer_ - dt);
    }

    // --- 横移動（入力一定速度） ---
    // 方向入力から目標速度を決定（地上/空中で同一）
    int inputX = (right ? 1 : 0) - (left ? 1 : 0);
    if (inputX != 0) {
        // 見た目の向き更新
        LRDirection newDir = (inputX > 0) ? LRDirection::kRight : LRDirection::kLeft;
        if (lrDirection_ != newDir) {
            lrDirection_ = newDir;
            turnFirstRotationY_ = transform_.rotate.y;
            turnTimer_ = kTimeTurn;
        }
    }

    // 最高速に素早くスナップさせる
    const float targetX = static_cast<float>(inputX) * kLimitRunSpeed;
    if (horizontalControlLockTimer_ <= 0.0f) {
        const float alpha = std::clamp(dt / kTimeToFullRun, 0.0f, 1.0f);
        velocity_.x += (targetX - velocity_.x) * alpha;
        velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
    }
    // ロック中は壁ジャン初速を尊重（クランプのみ）
    else {
        velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
    }

    // --- ジャンプ ---
    // 地上 or コヨーテジャンプ
    if ((onGround_ || coyoteCounter_ > 0) && jumpTriggered) {
        velocity_.y = kJumpAcceleration;
        onGround_ = false;
        coyoteCounter_ = 0;
        jumpBufferCounter_ = 0;
    }
    // 壁ジャンプ
    else if (!onGround_ && jumpTriggered && (isTouchingWall_ || wallCoyoteCounter_ > 0)) {
        const int dir = (lastWallDir_ == 0)
            ? ((right && !left) ? +1 : (left && !right) ? -1 : +1)
            : lastWallDir_;
        velocity_.y = kWallJumpVertical;
        velocity_.x = (dir > 0) ? -kWallJumpHorizontal : +kWallJumpHorizontal;

        // 向きも反転
        LRDirection newDir = (dir > 0) ? LRDirection::kLeft : LRDirection::kRight;
        if (lrDirection_ != newDir) {
            lrDirection_ = newDir;
            turnFirstRotationY_ = transform_.rotate.y;
            turnTimer_ = kTimeTurn;
        }

        // 壁ジャン後は二段ジャンプをリセット
        airJumpsLeft_ = kMaxAirJumps;

        // 入力ロック開始（壁ジャン初速を活かす）
        horizontalControlLockTimer_ = kWallJumpHorizLockTime;

        // 壁ジャンを行ったので空中ダッシュを再度使えるようにする（リセット）
        dashUsed_ = false;

        // 状態クリア
        onGround_ = false;
        coyoteCounter_ = 0;
        jumpBufferCounter_ = 0;
        isTouchingWall_ = false;
        wallCoyoteCounter_ = 0;
    }
    // 二段ジャンプ
    else if (!onGround_ && jumpTriggered && airJumpsLeft_ > 0) {
        velocity_.y = kJumpAcceleration;
        --airJumpsLeft_;
        coyoteCounter_ = 0;
        jumpBufferCounter_ = 0;
    }

    // ジャンプカット（ボタン離し）
    const bool jumpHeld = jumpDown;
    if (!jumpHeld && velocity_.y > 0.0f) {
        velocity_.y *= kJumpCutFactor;
    }

    // 重力（空中のみ）
    ApplyGravity();

    // 押下状態の保存
    jumpHeldPrev_ = jumpDown;
}

void Player::ApplyGravity() {
    if (!onGround_) {
        float g = (velocity_.y <= 0.0f) ? kgravityAcceleration * kFallGravityScale : kgravityAcceleration;
        velocity_ = Math::Add(velocity_, Vector3(0.0f, -g, 0.0f));
        velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
    }
}

/*
 * BehaviorMoveUpdate
 * 目的: 速度に基づく移動を、マップチップとの衝突でクリップしつつ反映する。
 * 手順: 1) 旋回演出 2) 衝突解決（Y→X） 3) 接触後処理 4) 行列更新
 */
void Player::BehaviorMoveUpdate() {
    // 1) 見た目の向き補間
    TurningControl();

    // 2) 軸分離で衝突解決
    CollisionMapInfo info{};
    info.amountMove = velocity_;
    CollisionDetection(info);

    // 3) 反映と接触後処理
    MoveAccordingly(info);
    ContactCeiling(info);
    ContactGround(info);
    ContactWall(info);

    // 4) 行列更新（ロジック→描画へ）
    UpdateMatrix();
}

// ===== マップ衝突 =====
void Player::CollisionDetection(CollisionMapInfo& info) {
    Vector3 base = transform_.translate;

    float dy = ResolveVerticalFrom(base, info.amountMove.y, info);
    base.y += dy;

    float dx = ResolveHorizontalFrom(base, info.amountMove.x, info);

    info.amountMove = Vector3{ dx, dy, 0.0f };
}

/*
 * ResolveVerticalFrom
 * 目的: Y 移動 dy を、頭/足の左右2点サンプルで検出したブロックに対してクリップする。
 * 方法: 上昇(頭): rect.bottom と自機 top の距離で dy を短縮（-kMBlank）。
 * 下降(足): rect.top と自機 bottom の距離で dy を伸長（+kMBlank）。
 * 対応: (旧) MapCollisionTop / MapCollisionBottom の統合。
 */
float Player::ResolveVerticalFrom(const Vector3& base, float dy, CollisionMapInfo& info) const {
    if (dy == 0.0f) { return 0.0f; }

    const float hx = kWidth * 0.5f;
    const float hy = kHeight * 0.5f;

    float allowed = dy;
    if (dy > 0.0f) {
        // 上昇: 新しい Y 位置で左右の「頭の点」を調べる
        const float topNew = base.y + dy + hy;
        Vector3 pL{ base.x - hx, topNew, 0.0f };
        Vector3 pR{ base.x + hx, topNew, 0.0f };

        MapChipField::IndexSet idx;
        MapChipField::Rect r;
        if (IsSolidAt(pL, &idx, &r)) {
            float cand = (r.bottom - kMBlank) - (base.y + hy);
            allowed = std::min(allowed, cand);
            info.isContactCeiling = true;
        }
        if (IsSolidAt(pR, &idx, &r)) {
            float cand = (r.bottom - kMBlank) - (base.y + hy);
            allowed = std::min(allowed, cand);
            info.isContactCeiling = true;
        }
    } else {
        // 下降: 新しい Y 位置で左右の「足の点」を調べる
        const float botNew = base.y + dy - hy;
        Vector3 pL{ base.x - hx, botNew, 0.0f };
        Vector3 pR{ base.x + hx, botNew, 0.0f };

        MapChipField::IndexSet idx;
        MapChipField::Rect r;
        if (IsSolidAt(pL, &idx, &r)) {
            float cand = (r.top + kMBlank) - (base.y - hy);
            allowed = std::max(allowed, cand);
            info.isContactGround = true;
        }
        if (IsSolidAt(pR, &idx, &r)) {
            float cand = (r.top + kMBlank) - (base.y - hy);
            allowed = std::max(allowed, cand);
            info.isContactGround = true;
        }
    }
    return allowed;
}

/*
 * ResolveHorizontalFrom
 * 目的: X 移動 dx を、左右端の上下2点サンプルで検出したブロックに対してクリップする。
 * 方法: 右移動: rect.left と自機 right の距離で dx を短縮（-kMBlank）。
 * 左移動: rect.right と自機 left の距離で dx を伸長（+kMBlank）。
 * 対応: (旧) MapCollisionRight / MapCollisionLeft の統合。
 */
float Player::ResolveHorizontalFrom(const Vector3& base, float dx, CollisionMapInfo& info) const {
    if (dx == 0.0f) { return 0.0f; }

    const float hx = kWidth * 0.5f;
    const float hy = kHeight * 0.5f;

    float allowed = dx;
    if (dx > 0.0f) {
        // 右移動: 新しい X 位置で上下の「右端の点」を調べる
        const float rightNew = base.x + dx + hx;
        Vector3 pT{ rightNew, base.y + hy, 0.0f };
        Vector3 pB{ rightNew, base.y - hy, 0.0f };

        MapChipField::IndexSet idx;
        MapChipField::Rect r;
        if (IsSolidAt(pT, &idx, &r)) {
            float cand = (r.left - kMBlank) - (base.x + hx);
            allowed = std::min(allowed, cand);
            info.isContactWall = true;
            info.wallDir = +1; // 右側に壁
        }
        if (IsSolidAt(pB, &idx, &r)) {
            float cand = (r.left - kMBlank) - (base.x + hx);
            allowed = std::min(allowed, cand);
            info.isContactWall = true;
            info.wallDir = +1; // 右側に壁
        }
    } else {
        // 左移動: 新しい X 位置で上下の「左端の点」を調べる
        const float leftNew = base.x + dx - hx;
        Vector3 pT{ leftNew, base.y + hy, 0.0f };
        Vector3 pB{ leftNew, base.y - hy, 0.0f };

        MapChipField::IndexSet idx;
        MapChipField::Rect r;
        if (IsSolidAt(pT, &idx, &r)) {
            float cand = (r.right + kMBlank) - (base.x - hx);
            allowed = std::max(allowed, cand);
            info.isContactWall = true;
            info.wallDir = -1; // 左側に壁
        }
        if (IsSolidAt(pB, &idx, &r)) {
            float cand = (r.right + kMBlank) - (base.x - hx);
            allowed = std::max(allowed, cand);
            info.isContactWall = true;
            info.wallDir = -1; // 左側に壁
        }
    }
    return allowed;
}

void Player::MoveAccordingly(const CollisionMapInfo& info) {
    transform_.translate = Math::Add(transform_.translate, info.amountMove);
    // ダッシュエフェクトモデルはプレイヤ位置に追従
    dashEffectTransform_.translate = transform_.translate;
}

/*
 * IsSolidAt
 * 目的: 座標 p の属するタイルがブロックか判定し、必要ならその Index/Rect を返す。
 * 注意: 範囲外は MapChipField 側で kBlank を返す想定。
 */
bool Player::IsSolidAt(const Vector3& p, MapChipField::IndexSet* outIdx, MapChipField::Rect* outRect) const {
    auto idx = mapChipField_->GetMapChipIndexSetByPosition(p);
    if (outIdx) { *outIdx = idx; }
    MapChipType t = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
    if (t == MapChipType::kBlock) {
        if (outRect) { *outRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex); }
        return true;
    }
    return false;
}

// ===== 接触後の後処理 =====
void Player::ContactCeiling(const CollisionMapInfo& info) {
    if (info.isContactCeiling && velocity_.y > 0.0f) {
        velocity_.y = 0.0f;
    }
}

void Player::ContactGround(const CollisionMapInfo& info) {
    // 空中→接地に遷移したフレーム
    if (!onGround_ && info.isContactGround) {
        // 着地直前に押していたら、着地フレームで即ジャンプ（バッファ）
        if (jumpBufferCounter_ > 0) {
            // 地上ジャンプ扱いなので、空中ジャンプ回数はここでリセット
            airJumpsLeft_ = kMaxAirJumps;
            velocity_.y = kJumpAcceleration;
            onGround_ = false;
            jumpBufferCounter_ = 0;
            coyoteCounter_ = 0;
            // 着地したため、ダッシュ使用フラグをリセット
            dashUsed_ = false;
            return;
        }
        // 通常の着地
        onGround_ = true;
        // 着地でダッシュフラグをリセット（地上で再びダッシュ可能にする）
        dashUsed_ = false;
        velocity_.x *= (1.0f - kAttenuationLanding);
        // 二段ジャンプリセット
        airJumpsLeft_ = kMaxAirJumps;
    }
    // 接地中だったが、このフレームは地面に触れていない＝離床
    else if (onGround_ && !info.isContactGround) {
        onGround_ = false;
    }

    // コヨーテ更新
    if (info.isContactGround && onGround_) {
        coyoteCounter_ = kCoyoteFrames;
    } else if (coyoteCounter_ > 0) {
        --coyoteCounter_;
    }
}

void Player::ContactWall(const CollisionMapInfo& info) {
    if (info.isContactWall) {
        velocity_.x *= (1.0f - kAttenuationWall);
    }

    // 壁接触状態と壁コヨーテ更新（空中時のみ）
    if (!onGround_ && info.isContactWall) {
        isTouchingWall_ = true;
        wallCoyoteCounter_ = kWallCoyoteFrames;
        if (info.wallDir != 0) {
            lastWallDir_ = info.wallDir; // +1=右, -1=左
        }
    } else {
        isTouchingWall_ = false;
        if (wallCoyoteCounter_ > 0) {
            --wallCoyoteCounter_;
        } else {
            lastWallDir_ = 0;
        }
    }

    // 壁スライド（Hollow Knight 風）：壁方向入力中は落下速度を強く制限
    // 条件: 空中 && 壁に触れている(もしくは直前まで触れていた) && 壁方向に入力
    int wallDir = (info.wallDir != 0) ? info.wallDir : lastWallDir_; // +1=右壁, -1=左壁
    bool pressingToward = false;
    if (wallDir != 0 && inputManager_) {
        if (wallDir > 0) {
            // 右壁に接触 → 右入力が「壁方向」
            pressingToward = inputManager_->IsKeyDown('D');
        } else {
            // 左壁に接触 → 左入力が「壁方向」
            pressingToward = inputManager_->IsKeyDown('A');
        }
    }
    if (!onGround_ && (isTouchingWall_ || info.isContactWall) && pressingToward && velocity_.y < 0.0f) {
        // 例: 通常終端 -0.36f → 壁スライド中は -0.12f までに抑える
        velocity_.y = std::max(velocity_.y, -kWallSlideMaxFallSpeed);
    }
}

// ===== 見た目の向き制御 =====
void Player::TurningControl() {
    if (turnTimer_ <= 0.0f) {
        transform_.rotate.y = (lrDirection_ == LRDirection::kRight)
            ? std::numbers::pi_v<float> / 2.0f
            : -std::numbers::pi_v<float> / 2.0f;
        return;
    }
    // 簡易な固定Δt補間（60fps想定）
    const float dt = 1.0f / 60.0f;
    float t = std::clamp(1.0f - (turnTimer_ / kTimeTurn), 0.0f, 1.0f);
    float target = (lrDirection_ == LRDirection::kRight)
        ? std::numbers::pi_v<float> / 2.0f
        : -std::numbers::pi_v<float> / 2.0f;
    transform_.rotate.y = Lerp(turnFirstRotationY_, target, EaseOutSine(t));
    turnTimer_ = std::max(0.0f, turnTimer_ - dt);
}

/*
 * UpdateMatrix
 * 役割: Player 自身のワールド行列を更新し、Transform を model にセットする。
 * 備考: 現状は Y 回転のみを考慮（本プロジェクトの使用状況に一致）。X/Z を使う場合は拡張する。
 */
void Player::UpdateMatrix() {
    // S*Ry*T の簡易アフィン
    const Vector3& s = transform_.scale;
    const Vector3& r = transform_.rotate;
    const Vector3& t = transform_.translate;

    const float cy = std::cos(r.y);
    const float sy = std::sin(r.y);

    // 行列をゼロ初期化
    worldMatrix_ = {};
    // 回転(Y)とスケール
    worldMatrix_.m[0][0] = s.x * cy;
    worldMatrix_.m[0][1] = 0.0f;
    worldMatrix_.m[0][2] = s.x * -sy;
    worldMatrix_.m[0][3] = 0.0f;
    worldMatrix_.m[1][0] = 0.0f;
    worldMatrix_.m[1][1] = s.y;
    worldMatrix_.m[1][2] = 0.0f;
    worldMatrix_.m[1][3] = 0.0f;
    worldMatrix_.m[2][0] = s.z * sy;
    worldMatrix_.m[2][1] = 0.0f;
    worldMatrix_.m[2][2] = s.z * cy;
    worldMatrix_.m[2][3] = 0.0f;
    // 平行移動
    worldMatrix_.m[3][0] = t.x;
    worldMatrix_.m[3][1] = t.y;
    worldMatrix_.m[3][2] = t.z;
    worldMatrix_.m[3][3] = 1.0f;

    // 描画側 Transform に反映
    model_->SetTransform(transform_);
    attackEffectModel_->SetTransform(attackEffectTransform_);
}

// ===== 幾何ユーティリティ =====
Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
    const float hx = kWidth * 0.5f;
    const float hy = kHeight * 0.5f;
    switch (corner) {
    case kRightBottom: return Math::Add(center, Vector3{ +hx, -hy, 0.0f });
    case kLeftBottom:  return Math::Add(center, Vector3{ -hx, -hy, 0.0f });
    case kRightTop:    return Math::Add(center, Vector3{ +hx, +hy, 0.0f });
    case kLeftTop:     return Math::Add(center, Vector3{ -hx, +hy, 0.0f });
    default:           return center;
    }
}

// ===== 旧個別判定（参考用・未使用） =====
void Player::MapCollisionTop(CollisionMapInfo& info) { (void)info; }
void Player::MapCollisionBottom(CollisionMapInfo& info) { (void)info; }
void Player::MapCollisionRight(CollisionMapInfo& info) { (void)info; }
void Player::MapCollisionLeft(CollisionMapInfo& info) { (void)info; }

// ===== OnCollision =====
void Player::OnCollision(const IEnemy* enemy) {
    // 敵が死亡している、またはプレイヤーがダッシュ中なら何もしない
    if (enemy->IsDead() || IsDashing()) {
        return;
    }
    TakeDamage(enemy->GetDamage(), enemy->GetWorldPosition());
}

void Player::TakeDamage(int damage, const Vector3& enemyPosition) {
    // 無敵時間中はダメージを受けない
    if (invincibilityTimer_ > 0.0f) {
        return;
    }

    hp_ -= damage;
    if (hp_ <= 0) {
        hp_ = 0;
        isDead_ = true;
    }

    // ダメージを受けたフラグを立てる
    isJustDamaged_ = true;

    // --- ノックバック処理 ---
    // 敵との位置関係からノックバック方向を決定
    float knockbackDir = (transform_.translate.x > enemyPosition.x) ? 1.0f : -1.0f;
    velocity_.x = knockbackDir * kKnockbackHorizontal;
    velocity_.y = kKnockbackVertical; // 常に少し上に跳ねる

    // 無敵時間を開始
    invincibilityTimer_ = kInvincibilityDuration;
    // ダメージを受けた瞬間に赤くする
    model_->SetColor({ 1.0f, 0.5f, 0.5f, 1.0f });
}

// ===== 補助 =====
bool Player::IsDashing() const { return state_ && state_->IsDashing(); }
bool Player::IsAttacking() const { return state_ && state_->IsAttacking(); }

// ===== 位置・AABB =====
Vector3 Player::GetWorldPosition() {

    // ワールド座標を入れる変数
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得(ワールド座標)
    worldPos.x = worldMatrix_.m[3][0];
    worldPos.y = worldMatrix_.m[3][1];
    worldPos.z = worldMatrix_.m[3][2];

    return worldPos;
}

AABB Player::GetAABB() {
    const Vector3 worldPos = GetWorldPosition();
    AABB aabb;
    aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
    aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };
    return aabb;
}

AABB Player::GetAttackAABB() {
    const Vector3 worldPos = GetWorldPosition();
    AABB aabb;
    const float attackWidth = 1.0f;
    const float attackHeight = 1.0f;
    float offsetX = (lrDirection_ == LRDirection::kRight) ? (kWidth / 2.0f + attackWidth / 2.0f) : -(kWidth / 2.0f + attackWidth / 2.0f);

    Vector3 attackCenter = { worldPos.x + offsetX, worldPos.y, worldPos.z };

    aabb.min = { attackCenter.x - attackWidth / 2.0f, attackCenter.y - attackHeight / 2.0f, attackCenter.z - attackWidth / 2.0f };
    aabb.max = { attackCenter.x + attackWidth / 2.0f, attackCenter.y + attackHeight / 2.0f, attackCenter.z + attackWidth / 2.0f };
    return aabb;
}