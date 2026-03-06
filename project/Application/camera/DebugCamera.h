#pragma once

#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4x4.h"
#include "Core/Math/Geometry/Math.h"
#include "Engine/Platform/Input/InputManager.h"
#include "camera/Camera.h"

/**
 * @class DebugCamera
 * @brief デバッグ目的で自由に操作できるカメラ
 *
 * キーボード入力により、シーン内を自由に移動・回転できます。
 */
class DebugCamera {
private: //メンバ変数
    // カメラ注視点
    Vector3 target_{};
    // カメラ注視点までの距離(ピボット回転)
    float distance_{ 10.0f };
    // 入力クラスのポインタ
    InputManager* input_ = nullptr;
    // カメラ
    Camera camera_{};

public: //メンバ関数
    /**
     * @brief 初期化処理
     * @param input InputManagerのポインタ
     * @param windowWidth ウィンドウの幅
     * @param windowHeight ウィンドウの高さ
     */
    void Initialize(InputManager* input, int windowWidth, int windowHeight);

    /**
     * @brief 更新処理
     */
    void Update();

    //ゲッター

    /**
     * @brief 内部で管理しているCameraオブジェクトを取得します
     * @return const Camera& カメラオブジェクト
     */
    const Camera& GetCamera() const { return camera_; }

    /**
     * @brief 内部で管理しているCameraオブジェクトを取得します
     * @return Camera& カメラオブジェクト
    */
    Camera& GetCamera() { return camera_; }
};

