#pragma once

#include "Framework/IScene.h"

#include "Renderer/Object3D/AnimationModel/AnimationModel.h"
#include "Irufemi.h"

#include <memory>
#include <vector>

// 前方宣言
class IrufemiEngine;
class Camera;
class DebugCamera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;

class DebugScene : public IScene {
public: // メンバ関数(ゲーム)
    ~DebugScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    void DrawDebugTab() override;
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

    std::unique_ptr<CylinderClass> cylinder_ = nullptr;
    bool isActiveCylinder_ = true;

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

    std::unique_ptr<GPUParticleSystem> gpuParticle_ = nullptr;
    bool isActiveGPUParticle_ = false;

    std::unique_ptr<VoxelParticleSystem> voxelParticle_ = nullptr;
    bool isActiveVoxelParticle_ = false;

    std::unique_ptr<EffectSystem> effect_ = nullptr;
    bool isActiveEffect_ = false;

    std::unique_ptr<AnimationModel> animatedCube_ = nullptr;
    bool isActiveAnimatedCube_ = false;

    std::unique_ptr<AnimationModel> walk_ = nullptr;
    bool isActiveWalk_ = false;

    std::unique_ptr<AnimationModel> sneakWalk_ = nullptr;
    bool isActiveSneakWalk_ = false;

    std::unique_ptr<Skybox> skybox_ = nullptr;

    // --- ライト ---
    bool isActiveSkybox_ = false;

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

