#pragma once

#include "scene/IScene.h"
#include <memory>
#include <vector>
#include "actors/enemy/Enemy.h"

// 前方宣言
class IrufemiEngine;
class InputManager;
class Camera;
class DebugCamera;
class Player;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;

class GameScene : public IScene {
public:
    GameScene();
    ~GameScene() override;

    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    void PauseUpdate() override;
    void PauseDraw() override;
    bool IsPausable() const override { return true; }

private:
    IrufemiEngine* engine_ = nullptr;
    std::unique_ptr<Camera> camera_ = nullptr;
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;
    bool debugMode_ = false;

    // ゲームオブジェクト
    std::unique_ptr<Player> player_ = nullptr;
    std::unique_ptr<Enemy> boss_ = nullptr;

    // ライト
    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;
};