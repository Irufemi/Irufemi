#pragma once

#include "scene/IScene.h"

#include <memory>
#include <vector>

#include"audio/Bgm.h"

class IrufemiEngine;

class Camera;
class DebugCamera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;

class SelectScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~SelectScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
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

private:
	Bgm bgmGameOver_;


};
