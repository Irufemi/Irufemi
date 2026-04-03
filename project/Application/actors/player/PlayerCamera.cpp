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

void PlayerCamera::UpdateDeathCamera(const Vector3& playerTranslate, float yaw, int deathTimer) {
    // ★調整：初期距離を少し離し、ズームアウトを速くする
    float distance = kCameraDistanceThirdPerson * 1.5f + (deathTimer * 0.3f);

    // ★変更：カメラをプレイヤーの背面方向（吹き飛ぶ方向）の「斜め前」に配置する
    // プレイヤーは yaw（向いていた方向）の背面（180度）へ飛ぶので、
    // カメラは yaw から少しずらした位置（斜め前）から捉えるのが良い。
    float cameraAngle = yaw + 0.5f; // 正面から少しずらす（約30度）

    // カメラの高さ用の角度（少し上から見下ろすように配置）
    float heightOffsetPitch = 0.3f; // 少し高めに調整 
    float cosPitch = std::cos(heightOffsetPitch);
    float sinPitch = std::sin(heightOffsetPitch);

    float cosAngle = std::cos(cameraAngle);
    float sinAngle = std::sin(cameraAngle);

    Vector3 cameraPos;
    // プレイヤーの斜め前上空にカメラを配置
    cameraPos.x = playerTranslate.x - (sinAngle * cosPitch * distance);
    cameraPos.y = playerTranslate.y + (sinPitch * distance);
    cameraPos.z = playerTranslate.z - (cosAngle * cosPitch * distance);

    // 地面にめり込まないように制限
    if (cameraPos.y < kCameraMinY) {
        cameraPos.y = kCameraMinY;
    }

    camera_->SetTranslate(cameraPos);

    // ★修正: カメラの向きを常にプレイヤーの少し上に向ける (LookAt)
    // 回転するモデル全体を捉えやすくするため、ターゲット位置を少し高く設定
    Vector3 lookAtTarget = {
        playerTranslate.x,
        playerTranslate.y + 1.5f,
        playerTranslate.z
    };

    Vector3 toPlayer = {
        lookAtTarget.x - cameraPos.x,
        lookAtTarget.y - cameraPos.y,
        lookAtTarget.z - cameraPos.z
    };

    // ベクトルからYaw角（左右の向き）を計算
    float lookYaw = std::atan2(toPlayer.x, toPlayer.z);

    // ベクトルからPitch角（上下の向き）を計算
    float horizontalDist = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
    float lookPitch = -std::atan2(toPlayer.y, horizontalDist);

    // 計算した角度をカメラに適用
    camera_->SetRotate({ lookPitch, lookYaw, 0.0f });
}