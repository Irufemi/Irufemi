#pragma once

#include "scene/IScene.h"
#include <memory>
#include <vector>

class IrufemiEngine;
class DebugCamera;
class Camera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;

/**
 * @class ResultScene
 * @brief ゲームの結果（クリア/ゲームオーバー）を表示するクラス
 *
 * ゲームの結果に応じてUIを表示し、入力に応じてステージ選択シーンへ戻ります。
 */
class ResultScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~ResultScene() override;
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
