#pragma once

#include "scene/IScene.h"
#include <memory>

class IrufemiEngine;
class Camera;
class Circle2D;
class PointLightClass;
class SpotLightClass;

class ResultScene : public IScene {
private: // メンバ関数(ゲーム)

private: // メンバ変数(ゲーム)

public: // メンバ関数(システム)
    ~ResultScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // メンバ関数(システム)
    IrufemiEngine* engine_ = nullptr;
    std::unique_ptr<Camera> camera_;

    std::unique_ptr<PointLightClass> pointLight_;
    std::unique_ptr<SpotLightClass> spotLight_;
};
