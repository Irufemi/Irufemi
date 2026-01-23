#pragma once

#include "scene/IScene.h"

#include "3D/SphereClass.h"
#include "3D/ObjClass.h"
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

    std::unique_ptr<SphereClass> sphere_ = nullptr;

    std::unique_ptr<ObjClass> planeObj_ = nullptr;
    bool isActivePlaneObj_ = false;

    std::unique_ptr<ObjClass> planeGltf_ = nullptr;
    bool isActivePlaneGltf_ = false;

    std::unique_ptr<AnimationModel> animatedCube_ = nullptr;
    bool isActiveAnimatedCube_ = false;

    std::unique_ptr<AnimationModel> walk_ = nullptr;
    bool isActiveWalk_ = false;

    std::unique_ptr<AnimationModel> sneakWalk_ = nullptr;
    bool isActiveSneakWalk_ = false;

    std::unique_ptr<AnimationModel> animationNode_ = nullptr;
    bool isActiveAnimationNode_ = false;

    std::unique_ptr<AnimationModel> animationNodeMisc_ = nullptr;
    bool isActiveAnimationNodeMisc_ = false;

    std::unique_ptr<ObjClass> meshPrimitives_ = nullptr;
    bool isActiveMeshPrimitives_ = false;

    std::unique_ptr<ObjClass> meshPrimitiveVertexColor_ = nullptr;
    bool isActiveMeshPrimitiveVertexColor_ = false;

    std::unique_ptr<ObjClass> textureSampler_ = nullptr;
    bool isActiveTextureSampler_ = false;

    std::unique_ptr<ObjClass> materialAlphaBlend_ = nullptr;
    bool isActiveMaterialAlphaBlend_ = false;

    std::unique_ptr<AnimationModel> animationSkin_ = nullptr;
    bool isActiveAnimationSkin_ = false;

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

