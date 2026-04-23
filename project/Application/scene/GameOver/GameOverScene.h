#pragma once

#include "Framework/IScene.h"
#include "Renderer/Object2D/Sprite/Sprite.h"
#include <memory>
#include <vector>

class IrufemiEngine;
class DebugCamera;
class Camera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;
class ObjClass;

/**
 * @class GameOverScene
 * @brief ゲームの結果（クリア/ゲームオーバー）を表示するクラス
 *
 * ゲームの結果に応じてUIを表示し、入力に応じてステージ選択シーンへ戻ります。
 */
class GameOverScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~GameOverScene() override;
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
    void DrawDebugTab() override;


private: // メンバ関数(内部ヘルパ)

    // 「Push to Space」文字
    std::unique_ptr<ObjClass> textPushToSpace_ = nullptr;

    // 「GameOver...」文字
    std::unique_ptr<ObjClass> goTextG_ = nullptr;
    std::unique_ptr<ObjClass> goTextA_ = nullptr;
    std::unique_ptr<ObjClass> goTextM_ = nullptr;
    std::unique_ptr<ObjClass> goTextE1_ = nullptr;
    std::unique_ptr<ObjClass> goTextO_ = nullptr;
    std::unique_ptr<ObjClass> goTextV_ = nullptr;
    std::unique_ptr<ObjClass> goTextE2_ = nullptr;
    std::unique_ptr<ObjClass> goTextR_ = nullptr;
    std::unique_ptr<ObjClass> goTextDot_ = nullptr;

private: // メンバ変数(システム)
    // エンジン
    IrufemiEngine* engine_ = nullptr;
    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;
    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    bool debugMode_ = false;

    bool isChangingScene_ = false;
    bool isTransitionRequested_ = false;
    float transitionDelayTimer_ = 0.0f;
    bool isDrawPushToSpace_ = true;
    float animationTime_ = 0.0f;

    // ライト
    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;
};
