#include "ParticleEmitterComponent.h"
#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"

ParticleEmitterComponent::ParticleEmitterComponent() {}

ParticleEmitterComponent::~ParticleEmitterComponent() {}

void ParticleEmitterComponent::OnRegisterProperties() {
    RegisterProperty("Texture Path", &texturePath_);
    RegisterProperty("Emit On Awake", &emitOnAwake_);
    
    // 0: Sphere, 1: Beam, 2: Box, 3: Cylinder
    RegisterProperty("Emit Type (0-3)", &emitType_);
    RegisterProperty("Emit Count", &emitCountPerFrame_);
    RegisterProperty("Frequency", &emitFrequency_);
    RegisterProperty("Life Min", &lifeTimeMin_);
    RegisterProperty("Life Max", &lifeTimeMax_);
    RegisterProperty("Velocity", &velocity_);
    RegisterProperty("Radius", &radius_);
    RegisterProperty("Spread", &spread_);
    RegisterProperty("Direction", &direction_);
    RegisterProperty("Color", &color_);
    RegisterProperty("Start Scale", &startScale_);
    RegisterProperty("End Scale", &endScale_);
}

void ParticleEmitterComponent::Initialize() {
    particleSystem_ = std::make_unique<GPUParticleSystem>();
    
    // "resources/" を考慮する
    std::string fullPath = "resources/" + texturePath_;
    if (texturePath_.find("resources/") == 0) {
        fullPath = texturePath_;
    }
    particleSystem_->Initialize(fullPath);

    if (gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
    }

    if (emitOnAwake_) {
        Play();
    } else {
        Stop();
    }
}

void ParticleEmitterComponent::Update() {
    if (!particleSystem_) return;

    Vector3 pos = transform_ ? transform_->worldPosition_ : Vector3{0, 0, 0};

    // 毎フレームパラメータを反映させる（エディタで動的調整できるように）
    particleSystem_->SetColor(color_);
    particleSystem_->SetParticleLife(lifeTimeMin_, lifeTimeMax_);
    particleSystem_->SetParticleScale(startScale_, startScale_, endScale_, endScale_);

    // エミッターの形状とパラメータを設定
    switch (emitType_) {
    case 0: // Sphere
        particleSystem_->SetSphereEmitter(pos, radius_, emitCountPerFrame_, emitFrequency_);
        break;
    case 1: // Beam
        particleSystem_->SetBeamEmitter(pos, direction_, radius_, velocity_, spread_, emitCountPerFrame_, emitFrequency_);
        break;
    case 2: // Box (10.0f は仮のサイズ)
        particleSystem_->SetBoxEmitter(pos, {radius_, radius_, radius_}, emitCountPerFrame_, emitFrequency_);
        break;
    case 3: // Cylinder (height は仮)
        particleSystem_->SetCylinderEmitter(pos, direction_, radius_, radius_ * 2.0f, emitCountPerFrame_, emitFrequency_);
        break;
    }

    particleSystem_->Update();
}

void ParticleEmitterComponent::Draw() {
    if (particleSystem_) {
        particleSystem_->Draw();
    }
}

IRenderable* ParticleEmitterComponent::GetRenderable() {
    return reinterpret_cast<IRenderable*>(particleSystem_.get());
}

void ParticleEmitterComponent::Play() {
    if (particleSystem_) {
        particleSystem_->SetEmit(true);
    }
}

void ParticleEmitterComponent::Stop() {
    if (particleSystem_) {
        particleSystem_->SetEmit(false);
    }
}
