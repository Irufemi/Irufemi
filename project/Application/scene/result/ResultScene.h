#pragma once

#include "scene/IScene.h"
#include <memory>

class IrufemiEngine;
class DebugCamera;
class Camera;
class Circle2D;
struct PointLight;
struct SpotLight;
struct DirectionalLight;

class ResultScene : public IScene {
public:
    ~ResultScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private:
    IrufemiEngine* engine_ = nullptr;

    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;

    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;

    std::unique_ptr<PointLight> pointLight_ = nullptr;

    std::unique_ptr<SpotLight> spotLight_ = nullptr;

    bool debugMode = false;

    std::unique_ptr<Circle2D> circle_;
};
