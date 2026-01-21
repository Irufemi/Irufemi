#pragma once

#include "scene/IScene.h"

#include "audio/Bgm.h"
#include "audio/Se.h"
#include "math/shape/LinePrimitive.h"
#include "2D/Sprite.h"
#include "3D/ObjClass.h"
#include "3D/SphereClass.h"
#include "3D/CubeClass.h"
#include "3D/PlaneClass.h"
#include "3D/CylinderClass.h"
#include "3D/TriangleClass.h"
#include "3D/particle/ParticleSystem.h"
#include "3D/effect/EffectSystem.h"
#include "3D/LineClass.h"
#include "3D/AnimationModel.h"

#include <memory>
#include <vector>

// 前方宣言
class IrufemiEngine;
class InputManager;
class Camera;
class DebugCamera;
class Sprite;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;

class CGScene : public IScene {
public: // メンバ関数(ゲーム)

public:
    ~CGScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    // このシーンはポーズ可能
    bool IsPausable() const override { return true; }

private: // メンバ関数(内部ヘルパ)

private: // メンバ変数(ゲーム)

    std::unique_ptr<AnimationModel> animationModel_animatedCube_ = nullptr;
    bool isActiveAnimationModel_animatedCube_ = false;

    std::unique_ptr<AnimationModel> animationModel_walk_ = nullptr;
    bool isActiveAnimationModel_walk_ = false;

    std::unique_ptr<AnimationModel> animationModel_sneakWalk_ = nullptr;
    bool isActiveAnimationModel_sneakWalk_ = false;

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
};

