#pragma once

#include "scene/IScene.h"
#include <memory>
#include <vector>

#include "StripeTransition.h"
#include "audio/Bgm.h"

class IrufemiEngine;
class DebugCamera;
class Camera;
class Sprite;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;

class ResultScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~ResultScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // メンバ関数(内部ヘルパ)

private: // メンバ変数(ゲーム)
    // ゲームクリア表示用スプライト
    std::unique_ptr<Sprite> gameClearSprite_ = nullptr;
    // ゲームオーバー表示用スプライト
    std::unique_ptr<Sprite> gameOverSprite_ = nullptr;
    // 現在表示する結果スプライト
    Sprite* currentResultSprite_ = nullptr;

    // トランジション
    std::unique_ptr<StripeTransition> stripeTransition_;
    bool isTransitioningToTitle_ = false;

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

    // Result BGM
    Bgm bgmResult_;
};
