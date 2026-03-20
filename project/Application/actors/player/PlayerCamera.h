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
    float mouseSensitivity_ = 5.0f;           // マウス感度
    float mouseSensitivityMultiplier_ = 1.0f; // マウス感度の倍率
    float cameraPitch_ = -0.1f;               // カメラの上下の角度（ピッチ）
    bool isCameraControlEnabled_ = true;      // カメラ操作の有効/無効フラグ

    ViewMode viewMode_ = ViewMode::kThirdPerson; // 視点モード
};