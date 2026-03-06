#pragma once

#include "Framework/IScene.h"
#include "Irufemi.h"

#include <memory>
#include <vector>

class Camera;
class DebugCamera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;

/**
 * @class SelectScene
 * @brief ステージ選択画面を管理するクラス
 *
 * プレイヤーがプレイするステージを選択し、決定に応じてゲームシーンへ遷移します。
 */
class SelectScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~SelectScene() override;

    /**
     * @brief 初期化処理
     * @param engine IrufemiEngineのポインタ
     */
    void Initialize(IrufemiEngine* engine) override;

    /**
     * @brief 毎フレームの更新処理
     */
    void Update() override;

    /**
     * @brief 描画処理
     */
    void Draw() override;

private: // メンバ関数(内部ヘルパ)

private: // メンバ変数(ゲーム)

private: // メンバ変数(システム)
    // エンジン
    IrufemiEngine* engine_ = nullptr;
    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;
    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    bool debugMode_ = false;

    // ライト
    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;
};
