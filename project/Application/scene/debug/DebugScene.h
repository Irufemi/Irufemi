#pragma once

#include "scene/IScene.h"

#include "audio/Bgm.h"
#include "audio/Se.h"
#include "math/shape/LinePrimitive.h"
#include "2D/Sprite.h"
#include "3D/ObjClass.h"
#include "3D/SphereClass.h"
#include "3D/PlaneClass.h"
#include "3D/TriangleClass.h"
#include "3D/ParticleClass.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "3D/CylinderClass.h"
#include <memory>
#include <vector>

class DebugScene : public IScene {

public: // メンバ関数
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // メンバ変数

    std::unique_ptr<Sprite> sprite_ = nullptr;
    bool isActiveSprite_ = false;

    std::unique_ptr<TriangleClass> triangle_ = nullptr;
    bool isActiveTriangle_ = false;

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

    std::unique_ptr<ParticleClass> particle_ = nullptr;
    bool isActiveParticle_ = false;

    std::unique_ptr<Sprite> imguiSprite_;
    bool showDemoWindow = false;

    IrufemiEngine* engine_ = nullptr;

    std::unique_ptr<Camera> camera_ = nullptr;

    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;
    bool debugMode = false;

    int loadTexture = false;

    std::unique_ptr<PointLightClass> pointLight_ = nullptr;
    std::unique_ptr<SpotLightClass> spotLight_ = nullptr;
};

