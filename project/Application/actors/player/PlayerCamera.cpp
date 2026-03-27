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

    // ★追加: Vキーで一人称視点 / 三人称視点を切り替える
    if (input->IsKeyPressed('V')) {
        if (viewMode_ == ViewMode::kThirdPerson) {
            viewMode_ = ViewMode::kFirstPerson;
        } else {
            viewMode_ = ViewMode::kThirdPerson;
        }
    }

    // --- マウスによる視点操作 ---
    if (isCameraControlEnabled_) {
        Vector2 mouseDelta = input->GetMouseDelta();
        float sensitivityMult = mouseSensitivity_ * mouseSensitivityMultiplier_ * kMouseSensitivityBase;

        playerRotate.y += mouseDelta.x * sensitivityMult;
        cameraPitch_ += mouseDelta.y * sensitivityMult;

        if (viewMode_ == ViewMode::kThirdPerson) {
            // 正の値が見下ろし、負の値が見上げ
            if (cameraPitch_ > kMaxCameraPitchThirdPerson) cameraPitch_ = kMaxCameraPitchThirdPerson;
            if (cameraPitch_ < kMinCameraPitchThirdPerson) cameraPitch_ = kMinCameraPitchThirdPerson;
        } else {
            if (cameraPitch_ > kMaxCameraPitchFirstPerson) cameraPitch_ = kMaxCameraPitchFirstPerson;
            if (cameraPitch_ < kMinCameraPitchFirstPerson) cameraPitch_ = kMinCameraPitchFirstPerson;
        }
    }
}

void PlayerCamera::Update(const Vector3& playerTranslate, const Vector3& playerRotate, const Vector3& missileVibration) {
    Vector3 cameraPos = { 0.0f, 0.0f, 0.0f };
    Vector3 lookAtTarget = {
        playerTranslate.x,
        playerTranslate.y + 1.5f,
        playerTranslate.z
    };

    if (viewMode_ == ViewMode::kThirdPerson) {
        float distance = kCameraDistanceThirdPerson;
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

        if (cameraPos.y < kCameraMinY) {
            cameraPos.y = kCameraMinY;
        }

        camera_->SetTranslate(cameraPos);
        camera_->SetRotate({ cameraPitch_, playerRotate.y, 0.0f });
    } else {
        cameraPos.x = playerTranslate.x;
        cameraPos.y = 1.0f + (playerTranslate.y * kCameraJumpFollowRatio);
        cameraPos.z = playerTranslate.z;

        cameraPos.x += missileVibration.x * 0.5f;
        cameraPos.y += missileVibration.y * 0.5f;
        cameraPos.z += missileVibration.z * 0.5f;

        camera_->SetTranslate(cameraPos);
        camera_->SetRotate({ cameraPitch_, playerRotate.y, 0.0f });
    }
}