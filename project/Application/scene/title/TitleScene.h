#pragma once

#include "scene/IScene.h"

#include <memory>
#include <vector>

#include "2D/Sprite.h"

#pragma region takamura追加
#include "StripeTransition.h"
#pragma endregion takamura追加

class IrufemiEngine;
class Camera;
class DebugCamera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;

/// <summary>
/// タイトル
/// </summary>
class TitleScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~TitleScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // メンバ関数(内部ヘルパ)

private: // メンバ変数(ゲーム)

    /// Sprite
    // テキスト
    // 血管壊回
    std::unique_ptr<Sprite> textSprite_title_ = nullptr;
    // 押したらスタート
    std::unique_ptr<Sprite> textSprite_pushStart_ = nullptr;

#pragma region takamura追加
    std::unique_ptr<StripeTransition> stripeTransition_;
    bool isTransitioning = false;
    int transitionTimer = 0;
    int transitionTime = 90;
#pragma endregion takamura追加

private: // メンバ変数(システム)
    // エンジン
    IrufemiEngine* engine_ = nullptr;
    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;
    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    bool isChangingScene_ = false;

    bool debugMode_ = false;
    // ライト
    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;
};