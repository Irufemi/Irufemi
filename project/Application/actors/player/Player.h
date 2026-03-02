#pragma once

#include "math/Vector3.h"
#include "math/Matrix4x4.h"
#include "3D/ObjClass.h"
#include <memory>

// 前方宣言
class Camera;
class InputManager;
class IrufemiEngine;

/**
 * @class Player
 * @brief プレイヤーキャラクターを管理するクラス
 */
class Player {
public:
    enum class ViewMode {
        kFirstPerson, // 一人称
        kThirdPerson  // 三人称
    };

    // デストラクタ
    ~Player();

    /**
     * @brief 初期化処理
     * @param input InputManagerのポインタ
     * @param camera Cameraのポインタ
     * @param engine IrufemiEngineのポインタ
     */
    void Initialize(InputManager* input, Camera* camera, IrufemiEngine* engine);

    /**
     * @brief 更新処理
     */
    void Update();

    /**
     * @brief 描画処理
     */
    void Draw();

    // ゲッター
    const Vector3& GetTranslate() const { return translate_; }
    const Vector3& GetRotate() const { return rotate_; }

private:
    /**
     * @brief 移動処理
     */
    void HandleMovement();

    /**
     * @brief カメラ座標の更新
     */
    void UpdateCamera();

private:
    // 外部依存
    InputManager* input_ = nullptr;
    Camera* camera_ = nullptr;
    IrufemiEngine* engine_ = nullptr;

    // 3Dモデル本体
    std::unique_ptr<ObjClass> obj_ = nullptr;

    // トランスフォーム
    Vector3 scale_ = { 0.3f, 0.3f, 0.3f };
    Vector3 rotate_ = { 0.0f, 0.0f, 0.0f };
    Vector3 translate_ = { 0.0f, 0.0f, 0.0f };

    // 移動用物理変数
    Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    ViewMode viewMode_ = ViewMode::kThirdPerson;
    bool isGrounded_ = true;

    // パラメータ
    const float kMoveSpeed = 0.2f;
    const float kJumpForce = 0.5f;
    const float kGravity = 0.02f;
};