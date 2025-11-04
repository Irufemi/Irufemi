#pragma once

#include "../IScene.h"

#include "audio/Bgm.h"
#include "audio/Se.h"
#include "math/shape/LinePrimitive.h"
#include "2D/Sprite.h"
#include "3D/ObjClass.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "3D/CylinderClass.h"
#include <memory>
#include <vector>

/// <summary>
/// タイトル
/// </summary>
class TitleScene : public IScene {

public: // メンバ関数
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // メンバ変数
    IrufemiEngine* engine_ = nullptr;

    std::unique_ptr<Camera> camera_ = nullptr;

    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;
    bool debugMode = false;

    std::unique_ptr<PointLightClass> pointLight_ = nullptr;
    std::unique_ptr<SpotLightClass> spotLight_ = nullptr;
};