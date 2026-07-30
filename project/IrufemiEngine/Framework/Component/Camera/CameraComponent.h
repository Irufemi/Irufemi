#pragma once
#include "../Component.h"
#include "Engine/Graphics/Camera/Camera.h"
#include <memory>

/**
 * @class CameraComponent
 * @brief 3D空間のカメラ（視点）をGameObjectとして管理するためのコンポーネント
 * @details 自身の TransformComponent の位置・回転とカメラの行列情報を同期します。
 */
class CameraComponent : public Component {
public:
    CameraComponent();
    ~CameraComponent() override;

    /**
     * @brief Initialize を実行する。
     */
    void Initialize() override;
    /**
     * @brief Update を実行する。
     */
    void Update() override;

    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override { return "CameraComponent"; }

    // --- ゲッター・セッター ---
    /**
     * @brief Camera を取得する。
     * @return 取得された Camera
     */
    std::shared_ptr<Camera> GetCamera() const { return camera_; }
    
    /**
     * @brief FovAngleY を取得する。
     * @return 取得された FovAngleY
     */
    float GetFovAngleY() const { return fovAngleY_; }
    /**
     * @brief FovAngleY を設定する。
     * @param[in] fov 設定する FovAngleY の値
     */
    void SetFovAngleY(float fov) { fovAngleY_ = fov; }

    /**
     * @brief NearZ を取得する。
     * @return 取得された NearZ
     */
    float GetNearZ() const { return nearZ_; }
    /**
     * @brief NearZ を設定する。
     * @param[in] nearZ 設定する NearZ の値
     */
    void SetNearZ(float nearZ) { nearZ_ = nearZ; }

    /**
     * @brief FarZ を取得する。
     * @return 取得された FarZ
     */
    float GetFarZ() const { return farZ_; }
    /**
     * @brief FarZ を設定する。
     * @param[in] farZ 設定する FarZ の値
     */
    void SetFarZ(float farZ) { farZ_ = farZ; }

protected:
    /**
     * @brief OnRegisterProperties を実行する。
     */
    void OnRegisterProperties() override;

private:
    std::shared_ptr<Camera> camera_ = nullptr; ///< 内包するカメラオブジェクト
    
    // シリアライズ・インスペクター編集用プロパティ
    float fovAngleY_ = 45.0f * 3.141592654f / 180.0f; ///< 垂直視野角 (ラジアン)
    float nearZ_ = 0.1f;                             ///< 近クリップ面
    float farZ_ = 1000.0f;                           ///< 遠クリップ面
    bool makeActive_ = true;                         ///< 起動時に自動でアクティブカメラにするか
};
