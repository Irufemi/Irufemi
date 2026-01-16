#pragma once

#include "scene/IScene.h"

// 環境物
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include <math/PointLight.h>
#include <math/SpotLight.h>
#include <math/DirectionalLight.h>

#include "audio/Bgm.h"
#include "audio/Se.h"
#include "math/shape/LinePrimitive.h"
#include "2D/Sprite.h"
#include "3D/ObjClass.h"
#include "3D/SphereClass.h"
#include "3D/CubeClass.h"
#include "3D/PlaneClass.h"
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

class DebugScene : public IScene {
public: // メンバ関数(ゲーム)

public:
    ~DebugScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    // このシーンはポーズ可能
    bool IsPausable() const override { return true; }

private: // メンバ関数(内部ヘルパ)

private: // メンバ変数(ゲーム)

    std::unique_ptr<Sprite> sprite_ = nullptr;
    bool isActiveSprite_ = false;

    std::unique_ptr<TriangleClass> triangle_ = nullptr;
    bool isActiveTriangle_ = false;

    std::unique_ptr<CubeClass> cube_ = nullptr;
    bool isActiveCube_ = false;

    std::unique_ptr<PlaneClass> plane_ = nullptr;
    bool isActivePlane_ = true;

    std::unique_ptr<SphereClass> sphere_ = nullptr;
    bool isActiveSphere_ = true;

    std::unique_ptr<ObjClass> obj_ = nullptr;
    bool isActiveObj_ = false;

    std::unique_ptr<ObjClass> utashTeapot_ = nullptr;
    bool isActiveUtashTeapot_ = false;

    std::unique_ptr<ObjClass> stanfordBunny_ = nullptr;
    bool isActiveStanfordBunny_ = false;

    std::unique_ptr<ObjClass> multiMesh_ = nullptr;
    bool isActiveMultiMesh_ = false;

    std::unique_ptr<ObjClass> multiMaterial_ = nullptr;
    bool isActiveMultiMaterial_ = false;

    std::unique_ptr<ObjClass> suzanne_ = nullptr;
    bool isActiveSuzanne_ = false;

    std::unique_ptr<ObjClass> fence_ = nullptr;
    bool isActiveFence_ = false;

    std::unique_ptr<ObjClass> terrain_ = nullptr;
    bool isActiveTerrain_ = false;

    std::unique_ptr<ParticleSystem> particle_ = nullptr;
    bool isActiveParticle_ = false;

    std::unique_ptr<EffectSystem> effect_ = nullptr;
    bool isActiveEffect_ = false;

    std::unique_ptr<AnimationModel> animationModel_ = nullptr;
    bool isActiveAnimationModel_ = false;

    std::unique_ptr<Sprite> imguiSprite_;
    bool showDemoWindow = false;

    std::unique_ptr<Line2DClass> line2D_ = nullptr;

    std::unique_ptr<Line3DClass> line3D_ = nullptr;

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
};

