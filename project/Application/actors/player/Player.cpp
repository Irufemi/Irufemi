#define NOMINMAX
#include "Player.h"

#include "function/Ease.h"
#include "contents/MapChipField.h"
#include "function/Math.h"
#include "engine/Input/InputManager.h"
#include "manager/DebugUI.h"
#include "PlayerState.h"
#include "3D/ObjClass.h"
#include "actors/enemy/IEnemy.h"
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

    // Transform 初期化(右向きで開始)
    transform_.translate = position;
    transform_.rotate = Vector3{ 0.0f, std::numbers::pi_v<float> / 2.0f, 0.0f };
    transform_.scale = Vector3{ 1.0f, 1.0f, 1.0f };
    attackEffectTransform_ = transform_;

    // 物理コンポーネントの初期化
    physics_ = std::make_unique<PlayerPhysics>();
    physics_->Initialize(this, &transform_, mapChipField_, inputManager_);

    // 追加初期化
    hp_ = kMaxHP;
    invincibilityTimer_ = 0.0f;

    // 描画へ反映
    model_->SetTransform(transform_);

    // 初期状態は Root
    ChangeState(MakeRootState());

    // se(ダッシュ)
    se_dash_ = std::make_unique<Se>();
    se_dash_->Initialize("resources/se/se_dash.mp3");

    // se(攻撃)
    se_slash_ = std::make_unique<Se>();
    se_slash_->Initialize("resources/se/se_slash.mp3");
}

void Player::Update() {
#if defined USE_IMGUI
    ImGui::Begin("Player");
    ImGui::Text("State : %s", GetStateName());
    ImGui::Text("OnGround : %s", physics_->IsOnGround() ? "true" : "false");
    ImGui::Text("AirJumpsLeft : %d", physics_->GetAirJumpsLeft());
    ImGui::Text("TouchWall : %s (dir=%d, coyote=%d)", physics_->IsTouchingWall() ? "true" : "false", physics_->GetLastWallDir(), physics_->GetWallCoyoteCounter());
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
        physics_->BehaviorMoveUpdate();
    } else {
        // 攻撃中でも重力と行列更新は行う
        physics_->ApplyGravity();
        physics_->BehaviorMoveUpdate(); // 旋回と衝突判定のみ
    }
    UpdateMatrix();
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

const Vector3& Player::GetVelocity() const {
    return physics_->GetVelocity();
}

void Player::SetMapChipField(MapChipField* mapChipField) {
    mapChipField_ = mapChipField;
    if (physics_) {
        physics_->Initialize(this, &transform_, mapChipField_, inputManager_);
    }
}

/*
 * UpdateMatrix
 * 役割: Player 自身のワールド行列を更新し、Transform を model にセットする。
 * 備考: 現状は Y 回転のみを考慮(本プロジェクトの使用状況に一致)。X/Z を使う場合は拡張する。
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

// ===== OnCollision =====
void Player::OnCollision(const IEnemy* enemy) {
    // 敵が死亡している、またはプレイヤーがダッシュ中なら何もしない
    if (enemy->IsDead() || IsDashing()) {
        return;
    }
    TakeDamage(enemy->GetDamage(), enemy->GetWorldPosition());
}

void Player::TakeDamage(const int& damage, const Vector3& enemyPosition) {
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
    Vector3 knockbackVelocity = { knockbackDir * kKnockbackHorizontal, kKnockbackVertical, 0.0f };
    physics_->SetVelocity(knockbackVelocity);

    // 無敵時間を開始
    invincibilityTimer_ = kInvincibilityDuration;
    // ダメージを受けた瞬間に赤くする
    model_->SetColor({ 1.0f, 0.5f, 0.5f, 1.0f });
}

// ===== 補助 =====
bool Player::IsDashing() const { return state_ && state_->IsDashing(); }
bool Player::IsAttacking() const { return state_ && state_->IsAttacking(); }

// ===== 位置・AABB =====
Vector3 Player::GetWorldPosition() const {

    // ワールド座標を入れる変数
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得(ワールド座標)
    worldPos.x = worldMatrix_.m[3][0];
    worldPos.y = worldMatrix_.m[3][1];
    worldPos.z = worldMatrix_.m[3][2];

    return worldPos;
}

AABB Player::GetAABB() const {
    const Vector3 worldPos = GetWorldPosition();
    AABB aabb;
    aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
    aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };
    return aabb;
}

AABB Player::GetAttackAABB()const {
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