#pragma once

#include "scene/IScene.h"

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

class Camera;
class DebugCamera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;

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

    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;

    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;

    std::unique_ptr<PointLight> pointLight_ = nullptr;

    std::unique_ptr<SpotLight> spotLight_ = nullptr;

    bool debugMode = false;
};