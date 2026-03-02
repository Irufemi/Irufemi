#include "Player.h"

#include "engine/Input/InputManager.h"
#include "camera/Camera.h"
#include "function/Math.h"
#include "engine/IrufemiEngine.h"
#include <Windows.h>

void Player::Initialize(InputManager* input, Camera* camera, IrufemiEngine* engine) {
    input_ = input;
    camera_ = camera;
    engine_ = engine;

    // ※ ここでエンジンを通してモデルとテクスチャがロードされていることを前提とします。
    // 必要に応じて engine_->GetModelManager()->Load(...) などを呼び出してください。

    scale_ = { 1.0f, 1.0f, 1.0f };
    translate_ = { 0.0f, 0.0f, 0.0f };
    rotate_ = { 0.0f, 0.0f, 0.0f };
    velocity_ = { 0.0f, 0.0f, 0.0f };
    viewMode_ = ViewMode::kThirdPerson;
    isGrounded_ = true;
}

void Player::Update() {
    // 各種ハンドル操作
    HandleMovement();
    HandleAttack();
    HandleViewSwitch();

    // カメラへの反映
    UpdateCamera();
}

void Player::Draw() {
    // プレイヤーのモデル描画処理
    // IrufemiEngineの描画機能を使用して自らを描画します。
    // 例: engine_->GetModelManager()->DrawModel("player_model", translate_, rotate_, scale_);

    // 現状は座標のログ出し、またはエンジン側の簡易描画機能の呼び出しを想定
}

void Player::HandleMovement() {
    Vector3 move = { 0.0f, 0.0f, 0.0f };

    // キー入力による前後左右移動
    if (input_->IsKeyDown('W')) move.z += 1.0f;
    if (input_->IsKeyDown('S')) move.z -= 1.0f;
    if (input_->IsKeyDown('A')) move.x -= 1.0f;
    if (input_->IsKeyDown('D')) move.x += 1.0f;

    // 正規化して移動速度を適用
    if (move.x != 0.0f || move.z != 0.0f) {
        move = Math::Normalize(move);
        translate_.x += move.x * kMoveSpeed;
        translate_.z += move.z * kMoveSpeed;

        // 移動方向に体を向ける（一人称でない場合）
        if (viewMode_ == ViewMode::kThirdPerson) {
            rotate_.y = std::atan2(move.x, move.z);
        }
    }

    // ジャンプ処理
    if (isGrounded_) {
        if (input_->IsKeyPressed(VK_SPACE)) {
            velocity_.y = kJumpForce;
            isGrounded_ = false;
        }
    } else {
        // 重力落下
        velocity_.y -= kGravity;
        translate_.y += velocity_.y;

        // 地面(y=0)に着地したかの判定
        if (translate_.y <= 0.0f) {
            translate_.y = 0.0f;
            velocity_.y = 0.0f;
            isGrounded_ = true;
        }
    }
}

void Player::HandleAttack() {
    // Pキーで攻撃
    if (input_->IsKeyPressed('P')) {
        OutputDebugStringA("Player Attack!\n");
    }
}

void Player::HandleViewSwitch() {
    // 'V'キーで視点切り替え
    if (input_->IsKeyPressed('V')) {
        if (viewMode_ == ViewMode::kThirdPerson) {
            viewMode_ = ViewMode::kFirstPerson;
        } else {
            viewMode_ = ViewMode::kThirdPerson;
        }
    }
}

void Player::UpdateCamera() {
    if (!camera_) return;

    Vector3 cameraPos;
    if (viewMode_ == ViewMode::kThirdPerson) {
        // 三人称：プレイヤーの後ろ上方から見下ろす
        cameraPos.x = translate_.x;
        cameraPos.y = translate_.y + 5.0f;
        cameraPos.z = translate_.z - 10.0f;
        camera_->SetRotate({ 0.4f, 0.0f, 0.0f }); // 少し下を向く
    } else {
        // 一人称：プレイヤーの目線の位置
        cameraPos.x = translate_.x;
        cameraPos.y = translate_.y + 1.5f;
        cameraPos.z = translate_.z;
        camera_->SetRotate(rotate_); // プレイヤーと同じ向き
    }

    camera_->SetTranslate(cameraPos);
}