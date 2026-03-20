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

    /**
     * @brief 初期化処理
     * @param camera 制御するCameraポインタ
     */
    void Initialize(Camera* camera);

    /**
     * @brief マウス・キーボード入力によるカメラ操作（視点移動・切り替え）
     * @param input InputManagerポインタ
     * @param playerRotate プレイヤーの回転（参照渡しで更新される）
     */
    void UpdateInput(InputManager* input, Vector3& playerRotate);

    /**
     * @brief カメラの座標と回転をプレイヤーに追従させる
     * @param playerTranslate プレイヤーの座標
     * @param playerRotate プレイヤーの回転
     * @param missileVibration 武器クラスからのミサイル振動（カメラを揺らすため）
     */
    void Update(const Vector3& playerTranslate, const Vector3& playerRotate, const Vector3& missileVibration);

    // --- ゲッター ---
    float GetCameraPitch() const { return cameraPitch_; }
    ViewMode GetViewMode() const { return viewMode_; }
    bool IsFirstPerson() const { return viewMode_ == ViewMode::kFirstPerson; }
    bool IsCameraControlEnabled() const { return isCameraControlEnabled_; }

    // --- ImGui調整用のポインタ取得 ---
    float* GetMouseSensitivityPtr() { return &mouseSensitivity_; }
    float* GetMouseSensitivityMultiplierPtr() { return &mouseSensitivityMultiplier_; }
    bool* GetCameraControlEnabledPtr() { return &isCameraControlEnabled_; }

private:
    Camera* camera_ = nullptr;

    // --- カメラ・マウス操作用パラメータ ---
    float mouseSensitivity_ = 5.0f;
    float mouseSensitivityMultiplier_ = 1.0f;
    float cameraPitch_ = -0.1f;
    bool isCameraControlEnabled_ = true;
    ViewMode viewMode_ = ViewMode::kThirdPerson;

    // --- マジックナンバーを定数化 ---
    static constexpr float kMouseSensitivityBase = 0.001f;
    static constexpr float kMaxCameraPitchThirdPerson = 0.25f;
    static constexpr float kMinCameraPitchThirdPerson = -0.3f;

    // 一人称視点用の定数（必要に応じて調整）
    static constexpr float kMaxCameraPitchFirstPerson = 0.8f;
    static constexpr float kMinCameraPitchFirstPerson = -0.8f;

    static constexpr float kCameraDistanceThirdPerson = 5.0f;
    static constexpr float kCameraMinY = 0.2f;
    static constexpr float kCameraJumpFollowRatio = 0.5f;
};