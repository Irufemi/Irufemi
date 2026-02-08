#pragma once

#include "math/Vector3.h"
#include "math/Matrix4x4.h"
#include "function/Math.h"
#include "engine/Input/InputManager.h"
#include "camera/Camera.h"

/**
 * @class DebugCamera
 * @brief デバッグ目的で自由に操作できるカメラ
 *
 * キーボード入力により、シーン内を自由に移動・回転できます。
 */
class DebugCamera {
private: //メンバ変数
    // カメラ注視点までの距離(ピボット回転)
    float distance_{};
    // 入力クラスのポインタ
    InputManager* input_ = nullptr;
    // スケーリング
    float scaleX_ = 1.0f;
    float scaleY_ = 1.0f;
    // カメラ
    Camera camera_{};
    // 回転行列
    Matrix4x4 matRot_{};

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
    const Camera& GetCamera() { return camera_; }
};

