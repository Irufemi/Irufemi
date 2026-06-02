#pragma once
#include "../Component.h"
#include <string>
#include <memory>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"

#include "Renderer/ParticleGPU/GPUParticleManager.h"

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
    
    IRenderable* GetRenderable() override { return nullptr; }

    std::string GetComponentName() const override { return "ParticleEmitterComponent"; }
    void OnRegisterProperties() override;

    void Play();
    void Stop();
    
    /**
     * @brief 1回だけ大量のパーティクルを放出する（爆発など）
     * @param count 放出するパーティクルの数
     */
    void EmitBurst(int count);

private:
    std::string texturePath_ = "circle.png";
    bool emitOnAwake_ = true;
    
    // 挙動プリセット (0: Custom, 1: Explosion, 2: Spark, 3: Smoke, 4: Magic)
    int particleType_ = 0;
    
    // エミッターの基本パラメータ
    int emitType_ = 0; // 0: Sphere, 1: Beam, 2: Box, 3: Cylinder
    int emitCountPerFrame_ = 10;
    float emitFrequency_ = 0.1f;
    float lifeTimeMin_ = 0.5f;
    float lifeTimeMax_ = 1.0f;
    float velocity_ = 1.0f;
    float radius_ = 1.0f;
    float spread_ = 0.1f;
    
    // 物理挙動パラメータ
    float gravity_ = 0.0f;
    float damping_ = 0.0f;
    
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector3 startScale_ = { 1.0f, 1.0f, 1.0f };
    Vector3 endScale_ = { 0.0f, 0.0f, 0.0f };
    Vector3 direction_ = { 0.0f, 0.0f, 1.0f };

    GPUParticleManager::EmitterHandle emitterHandle_;
    bool isPlaying_ = false;
    int burstCountPending_ = 0;
    TransformComponent* transform_ = nullptr;

};
