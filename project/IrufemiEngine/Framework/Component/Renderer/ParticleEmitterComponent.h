#pragma once
#include "../Component.h"
#include <string>
#include <memory>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"

class GPUParticleSystem;
class TransformComponent;

/**
 * @class ParticleEmitterComponent
 * @brief パーティクル（エフェクト）の放出を管理するコンポーネント
 */
class ParticleEmitterComponent : public Component {
public:
    ParticleEmitterComponent();
    ~ParticleEmitterComponent() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    
    IRenderable* GetRenderable() override;

    std::string GetComponentName() const override { return "ParticleEmitterComponent"; }
    void OnRegisterProperties() override;

    void Play();
    void Stop();

private:
    std::string texturePath_ = "effect/particle_tex.png";
    bool emitOnAwake_ = true;
    
    // エミッターの基本パラメータ
    int emitType_ = 0; // 0: Sphere, 1: Beam, 2: Box, 3: Cylinder
    int emitCountPerFrame_ = 10;
    float emitFrequency_ = 0.1f;
    float lifeTimeMin_ = 0.5f;
    float lifeTimeMax_ = 1.0f;
    float velocity_ = 1.0f;
    float radius_ = 1.0f;
    float spread_ = 0.1f;
    
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector3 startScale_ = { 1.0f, 1.0f, 1.0f };
    Vector3 endScale_ = { 0.0f, 0.0f, 0.0f };
    Vector3 direction_ = { 0.0f, 0.0f, 1.0f };

    std::unique_ptr<GPUParticleSystem> particleSystem_;
    TransformComponent* transform_ = nullptr;
};
