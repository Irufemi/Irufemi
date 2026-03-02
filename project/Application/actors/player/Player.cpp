#include "Player.h"

#include "engine/Input/InputManager.h"
#include "camera/Camera.h"
#include "function/Math.h"
#include "engine/IrufemiEngine.h"
#include <Windows.h>
#include <cmath>

// デストラクタ
Player::~Player() {
}

void Player::Initialize(InputManager* input, Camera* camera, IrufemiEngine* engine) {
    input_ = input;
    camera_ = camera;
    engine_ = engine;

    // --- モデルの生成と初期化 ---
    obj_ = std::make_unique<ObjClass>();
    // とりあえず確認用として、Bodyと同じモデルを指定
    // もし自前のplayer.obj等があるならここを書き換えてください
    obj_->Initialize(camera_, "enemy/body.obj");

}

void Player::Update() {
    // 1. 移動処理
    HandleMovement();

    // 2. 視点切り替え(Vキー)
    if (input_->IsKeyPressed('V')) {
        viewMode_ = (viewMode_ == ViewMode::kThirdPerson) ? ViewMode::kFirstPerson : ViewMode::kThirdPerson;
    }

    // 3. 3Dモデルのトランスフォームを更新
    if (obj_) {
        obj_->SetPosition(translate_);
        obj_->SetRotate(rotate_);
        obj_->SetScale(scale_);
		obj_->Debug("Player");
        obj_->Update();
    }

    // 4. カメラをプレイヤーに追従させる
    UpdateCamera();
}

void Player::Draw() {
    // モデルの描画
    if (obj_) {
        // 一人称視点のときは自分の体が見えると邪魔なので描画しない
        if (viewMode_ != ViewMode::kFirstPerson) {
            obj_->Draw();
        }
    }
}

void Player::HandleMovement() {
    Vector3 move = { 0.0f, 0.0f, 0.0f };

    // キー入力取得
    if (input_->IsKeyDown('W')) move.z += 1.0f;
    if (input_->IsKeyDown('S')) move.z -= 1.0f;
    if (input_->IsKeyDown('A')) move.x -= 1.0f;
    if (input_->IsKeyDown('D')) move.x += 1.0f;

    // 平面移動
    if (move.x != 0.0f || move.z != 0.0f) {
        move = Math::Normalize(move);
        translate_.x += move.x * kMoveSpeed;
        translate_.z += move.z * kMoveSpeed;

        // 三人称視点の時は移動方向を向く
        if (viewMode_ == ViewMode::kThirdPerson) {
            rotate_.y = std::atan2(move.x, move.z);
        }
    }

    // ジャンプと重力
    if (isGrounded_) {
        if (input_->IsKeyPressed(VK_SPACE)) {
            velocity_.y = kJumpForce;
            isGrounded_ = false;
        }
    } else {
        velocity_.y -= kGravity;
        translate_.y += velocity_.y;

        // 地面判定（簡易）
        if (translate_.y <= 0.0f) {
            translate_.y = 0.0f;
            velocity_.y = 0.0f;
            isGrounded_ = true;
        }
    }
}

void Player::UpdateCamera() {
    if (!camera_) return;

    Vector3 cameraPos;
    if (viewMode_ == ViewMode::kThirdPerson) {
        // 三人称：後ろから見下ろす
        cameraPos.x = translate_.x;
        cameraPos.y = translate_.y + 5.0f;
        cameraPos.z = translate_.z - 20.0f;
        camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
    } else {
        // 一人称：目線の高さ
        cameraPos.x = translate_.x;
        cameraPos.y = translate_.y + 1.8f;
        cameraPos.z = translate_.z;
        camera_->SetRotate({ -0.2f, 0.0f, 0.0f });
    }

    camera_->SetTranslate(cameraPos);
}