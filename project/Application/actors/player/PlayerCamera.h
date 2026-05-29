#pragma once

#include "Irufemi.h"

// 前方宣言
class Camera;
class InputManager;

class PlayerCamera {
public:
    enum class ViewMode {
        kFirstPerson, // 一人称
        kThirdPerson  // 三人称
    };

    PlayerCamera() = default;
    ~PlayerCamera() = default;

    void Initialize();
    void UpdateInput(InputManager* input, Vector3& playerRotate);
    void Update(const Vector3& playerTranslate, const Vector3& playerRotate, const Vector3& missileVibration, IrufemiEngine* engine);

    /**
     * @brief 死亡時（吹き飛び時）のドラマチックなカメラワーク
     * @param cameraPos カメラを配置する座標（敵の目線など）
     * @param playerTranslate 飛んでいくプレイヤーの座標
     * @param engine エンジンへのポインタ
     */
    void UpdateDeathCamera(const Vector3& cameraPos, const Vector3& playerTranslate, IrufemiEngine* engine);

    // ★追加: 視点を三人称に強制する
    void ForceThirdPerson() { viewMode_ = ViewMode::kThirdPerson; }

    // --- ゲッター ---
    float GetCameraPitch() const { return cameraPitch_; }
    ViewMode GetViewMode() const { return viewMode_; }
    bool IsFirstPerson() const { return viewMode_ == ViewMode::kFirstPerson; }
    bool IsCameraControlEnabled() const { return isCameraControlEnabled_; }

    // --- ImGui調整用のポインタ取得 ---
    float* GetMouseSensitivityPtr() { return &mouseSensitivity_; }
    float* GetMouseSensitivityMultiplierPtr() { return &mouseSensitivityMultiplier_; }
    bool* GetCameraControlEnabledPtr() { return &isCameraControlEnabled_; }

    // チュートリアル用：視点切り替えの許可フラグ
    void SetAllowViewSwitch(bool allow) { allowViewSwitch_ = allow; }

private:

    bool allowViewSwitch_ = true;


    float mouseSensitivity_ = 5.0f;
    float mouseSensitivityMultiplier_ = 1.0f;
    float cameraPitch_ = -0.1f;
    bool isCameraControlEnabled_ = true;
    ViewMode viewMode_ = ViewMode::kThirdPerson;

    static constexpr float kMouseSensitivityBase = 0.001f;
    static constexpr float kMaxCameraPitchThirdPerson = 0.25f;
    static constexpr float kMinCameraPitchThirdPerson = -0.3f;

    static constexpr float kMaxCameraPitchFirstPerson = 0.8f;
    static constexpr float kMinCameraPitchFirstPerson = -0.8f;

    static constexpr float kCameraDistanceThirdPerson = 7.0f;
    static constexpr float kCameraMinY = 0.2f;
    static constexpr float kCameraJumpFollowRatio = 0.5f;
    // カメラのめり込み防止（フィールドの壁が±100.0fなので少し内側で制限）
    static constexpr float kCameraFieldLimitXZ = 98.0f;
};