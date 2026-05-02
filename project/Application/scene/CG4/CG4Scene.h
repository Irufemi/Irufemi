#pragma once

#include "Framework/IScene.h"
#include <memory>
#include <vector>

class IrufemiEngine;
class Camera;
class DebugCamera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;
class Skybox;
class AnimationModel;
#include "Renderer/Object3D/Primitive/RingClass.h"
#include "Renderer/Effect/Effect.h"

/**
 * @class CG4Scene
 * @brief 学校課題用のCG4シーンを管理するクラス
 *
 * 新しい描画や実験的な実装を行うための独立したシーンです。
 */
class CG4Scene : public IScene {
public: // メンバ関数
    ~CG4Scene() override;

    /**
     * @brief 初期化処理
     * @param engine IrufemiEngineのポインタ
     */
    void Initialize(IrufemiEngine* engine) override;

    /**
     * @brief 毎フレームの更新処理
     */
    void Update() override;

    /**
     * @brief 描画処理
     */
    void Draw() override;

    /**
     * @brief デバッグUIの描画処理
     */
    void DrawDebugTab() override;

private: // メンバ変数
    IrufemiEngine* engine_ = nullptr;

    std::unique_ptr<Camera> camera_ = nullptr;
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;
    bool debugMode_ = false;

    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;

    bool isActiveSkybox_ = false;
    std::unique_ptr<Skybox> skybox_ = nullptr;

    bool isActiveAnimatedCube_ = false;
    std::unique_ptr<AnimationModel> animatedCube_ = nullptr;

    bool isActiveWalk_ = true;
    std::unique_ptr<AnimationModel> walk_ = nullptr;

    bool isActiveSneakWalk_ = false;
    std::unique_ptr<AnimationModel> sneakWalk_ = nullptr;

    bool isActiveEffect_ = false;
    std::unique_ptr<Effect> effect_ = nullptr;

    bool isActiveRing_ = false;
    std::unique_ptr<RingClass> testRing_ = nullptr;
};
