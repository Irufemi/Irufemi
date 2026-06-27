#pragma once

#include "Framework/BaseScene.h"

#include "Renderer/Object/3D/AnimationModel/AnimationModel.h"
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h"
#include "Renderer/Object/2D/Primitive/Primitive2DObject.h"
#include "Irufemi.h"
#include "Graphics/Data/LightningParams.h"

#include <memory>
#include <vector>
#include "Renderer/Object/Particle/ParticleObject.h"

// 前方宣言
class IrufemiEngine;

class DebugScene : public BaseScene {
public: // メンバ関数(ゲーム)
    ~DebugScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    void DrawDebugTab() override;

private: // メンバ関数(内部ヘルパ)

private: // メンバ変数(ゲーム)

    std::unique_ptr<Sprite> sprite_ = nullptr;
    bool isActiveSprite_ = false;

    // --- プリミティブ形状のテスト用（汎用クラスに統合済み） ---
    std::unique_ptr<Primitive3DObject> primitiveObj_ = nullptr;
    bool isActivePrimitiveObj_ = false;

    std::unique_ptr<Primitive2DObject> primitive2DObj_ = nullptr;
    bool isActivePrimitive2DObj_ = false;

    std::unique_ptr<StaticModelObject> obj_ = nullptr;
    bool isActiveObj_ = false;

    std::unique_ptr<StaticModelObject> utashTeapot_ = nullptr;
    bool isActiveUtashTeapot_ = false;

    std::unique_ptr<StaticModelObject> stanfordBunny_ = nullptr;
    bool isActiveStanfordBunny_ = false;

    std::unique_ptr<StaticModelObject> multiMesh_ = nullptr;
    bool isActiveMultiMesh_ = false;

    std::unique_ptr<StaticModelObject> multiMaterial_ = nullptr;
    bool isActiveMultiMaterial_ = false;

    std::unique_ptr<StaticModelObject> suzanne_ = nullptr;
    bool isActiveSuzanne_ = false;

    std::unique_ptr<StaticModelObject> fence_ = nullptr;
    bool isActiveFence_ = false;

    std::unique_ptr<StaticModelObject> terrain_ = nullptr;
    bool isActiveTerrain_ = false;


    std::unique_ptr<VoxelParticleSystem> voxelParticle_ = nullptr;
    bool isActiveVoxelParticle_ = false;

    // ハードコーディングされたGPUパーティクルのテスト用
    std::unique_ptr<ParticleObject> particleObj_ = nullptr;
    bool isActiveGPUParticle_ = false;

    std::unique_ptr<AnimationModel> animatedCube_ = nullptr;
    bool isActiveAnimatedCube_ = false;

    std::unique_ptr<AnimationModel> walk_ = nullptr;
    bool isActiveWalk_ = false;

    std::unique_ptr<AnimationModel> sneakWalk_ = nullptr;
    bool isActiveSneakWalk_ = false;

    std::unique_ptr<Skybox> skybox_ = nullptr;

    // --- ライト ---
    bool isActiveSkybox_ = false;


    // --- ImGuiデモ ---
    bool isActiveImGuiDemo_ = false;

    // --- 電撃エフェクトデモ ---
    bool isActiveLightningCrawl_ = false;
    std::unique_ptr<Primitive3DObject> lightningCylinder_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> lightningParamsResource_ = nullptr;
    LightningParams* lightningParamsData_ = nullptr;

private: // メンバ変数(システム)

#ifdef USE_IMGUI
    ImGuizmo::OPERATION gizmoOperation_ = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE gizmoMode_ = ImGuizmo::LOCAL;
#endif
};

