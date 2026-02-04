#pragma once

#include "scene/IScene.h"

#include <memory>
#include <vector>

#include "2D/Sprite.h"

#pragma region takamura追加
#include "StripeTransition.h"
#pragma endregion takamura追加

#include "audio/Se.h"
#include "audio/Bgm.h"

// 前方宣言
class IrufemiEngine;
class Camera;
class DebugCamera;
class ObjClass;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;
class ParticleSystem;

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
    void UpdateTextAnimation();

private: // メンバ変数(ゲーム)

    /// Sprite

    // 押したらスタート
    std::unique_ptr<Sprite> textSprite_pushStart_ = nullptr;
    int textAnimationTimer_ = 0;
    bool isTextAnimationFast_ = false;

#pragma region takamura追加
    std::unique_ptr<StripeTransition> stripeTransition_;
    bool isTransitioning = false;
    int transitionTimer = 0;
    int transitionTime = 90;
#pragma endregion takamura追加
 
    std::unique_ptr<ObjClass> titleObj_ = nullptr;

    std::vector<std::unique_ptr<ParticleSystem>> particleSystems_;

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

private:
    Se seDecision_;
	Bgm bgmTitle_;
    
};