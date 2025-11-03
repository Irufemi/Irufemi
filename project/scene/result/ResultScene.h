#pragma once

#include "scene/IScene.h"
#include <memory>

class IrufemiEngine;
class Camera;
class Circle2D;
class PointLightClass;
class SpotLightClass;

class ResultScene : public IScene {
public:
    ~ResultScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private:
    IrufemiEngine* engine_ = nullptr;
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Circle2D> circle_;

    // ワークアラウンド用ライト
    std::unique_ptr<PointLightClass> pointLight_;
    std::unique_ptr<SpotLightClass> spotLight_;
};
