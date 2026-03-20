#include "PlayerCamera.h"
#include "camera/Camera.h"
#include <cmath>
#include <Windows.h>

void PlayerCamera::Initialize(Camera* camera) {
    camera_ = camera;
    mouseSensitivity_ = 5.0f;
    mouseSensitivityMultiplier_ = 1.0f;
    cameraPitch_ = -0.1f;
    isCameraControlEnabled_ = true;
    viewMode_ = ViewMode::kThirdPerson;
}

void PlayerCamera::UpdateInput(InputManager* input, Vector3& playerRotate) {
    // F2キーでカメラ操作の有効/無効を切り替え
    if (input->IsKeyPressed(VK_F2)) {
        isCameraControlEnabled_ = !isCameraControlEnabled_;
    }

    // --- マウスによる視点操作 ---
    if (isCameraControlEnabled_) {
        Vector2 mouseDelta = input->GetMouseDelta();
        float sensitivityMult = mouseSensitivity_ * mouseSensitivityMultiplier_ * 0.001f;

        playerRotate.y += mouseDelta.x * sensitivityMult;
        cameraPitch_ += mouseDelta.y * sensitivityMult;

        if (viewMode_ == ViewMode::kThirdPerson) {
            // 正の値が見下ろし、負の値が見上げ
            if (cameraPitch_ > 0.25f) cameraPitch_ = 0.25f;
            if (cameraPitch_ < -0.3f) cameraPitch_ = -0.3f;
        } else {
            if (cameraPitch_ > 0.25f) cameraPitch_ = 0.25f;
            if (cameraPitch_ < -1.3f) cameraPitch_ = -1.3f;
        }
    }

    // --- 視点切り替え(Vキー) ---
    if (input->IsKeyPressed('V')) {
        viewMode_ = (viewMode_ == ViewMode::kThirdPerson) ? ViewMode::kFirstPerson : ViewMode::kThirdPerson;

        // 視点を切り替えた直後の補正
        if (viewMode_ == ViewMode::kThirdPerson) {
            if (cameraPitch_ > 0.25f) cameraPitch_ = 0.25f;
            if (cameraPitch_ < -0.3f) cameraPitch_ = -0.3f;
        }
    }
}

void PlayerCamera::Update(const Vector3& playerTranslate, const Vector3& playerRotate, const Vector3& missileVibration) {
    if (!camera_) return;

    Vector3 cameraPos;
    const float kCameraJumpFollowRatio = 0.8f;

    Vector3 lookAtTarget = {
        playerTranslate.x,
        playerTranslate.y + 1.5f,
        playerTranslate.z
    };

    if (viewMode_ == ViewMode::kThirdPerson) {
        float distance = 5.0f;
        float cosPitch = std::cos(cameraPitch_);
        float sinPitch = std::sin(cameraPitch_);
        float cosYaw = std::cos(playerRotate.y);
        float sinYaw = std::sin(playerRotate.y);

        cameraPos.x = lookAtTarget.x - (sinYaw * cosPitch * distance);
        cameraPos.y = lookAtTarget.y + (sinPitch * distance);
        cameraPos.z = lookAtTarget.z - (cosYaw * cosPitch * distance);

        // カメラにも発射時の振動を少しだけブレンドして、画面全体を揺らす
        cameraPos.x += missileVibration.x * 0.5f;
        cameraPos.y += missileVibration.y * 0.5f;
        cameraPos.z += missileVibration.z * 0.5f;

        if (cameraPos.y < 0.2f) {
            cameraPos.y = 0.2f;
        }

        camera_->SetTranslate(cameraPos);
        camera_->SetRotate({ cameraPitch_, playerRotate.y, 0.0f });
    } else {
        cameraPos.x = playerTranslate.x;
        cameraPos.y = 1.0f + (playerTranslate.y * kCameraJumpFollowRatio);
        cameraPos.z = playerTranslate.z;

        // 一人称視点でも揺らす
        cameraPos.x += missileVibration.x * 0.5f;
        cameraPos.y += missileVibration.y * 0.5f;
        cameraPos.z += missileVibration.z * 0.5f;

        if (cameraPos.y < 0.2f) {
            cameraPos.y = 0.2f;
        }

        camera_->SetTranslate(cameraPos);
        camera_->SetRotate({ cameraPitch_, playerRotate.y, 0.0f });
    }
}