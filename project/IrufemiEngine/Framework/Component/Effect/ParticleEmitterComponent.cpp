#include "ParticleEmitterComponent.h"
#include "Framework/GameObject.h"
#include "../TransformComponent.h"

ParticleEmitterComponent::ParticleEmitterComponent() {
    particleObj_ = std::make_unique<ParticleObject>();
}

ParticleEmitterComponent::~ParticleEmitterComponent() {
}

void ParticleEmitterComponent::OnRegisterProperties() {
    if (particleObj_) {
        particleObj_->RegisterProperties(this);
    }
}

void ParticleEmitterComponent::Initialize() {
    transform_ = gameObject_->GetComponent<TransformComponent>();
    particleObj_->Initialize();
}

void ParticleEmitterComponent::Update() {
    if (transform_) {
        particleObj_->SetPosition(transform_->worldPosition_);
    }

#ifdef _DEBUG
    // エディタからの値変更をリアルタイム反映させるため、常にDirtyフラグを立てる
    particleObj_->MarkDirty();
#endif
    
    particleObj_->Update();
}

void ParticleEmitterComponent::Draw() {
}

void ParticleEmitterComponent::Play() {
    particleObj_->Play();
}

void ParticleEmitterComponent::Stop() {
    particleObj_->Stop();
}

void ParticleEmitterComponent::EmitBurst(int count) {
    particleObj_->EmitBurst(count);
}

nlohmann::json ParticleEmitterComponent::Serialize() {
    nlohmann::json j = Component::Serialize();
    if (particleObj_) {
        nlohmann::json particleJson;
        particleObj_->Serialize(particleJson);
        j["ParticleData"] = particleJson;
    }
    return j;
}

void ParticleEmitterComponent::Deserialize(const nlohmann::json& j) {
    Component::Deserialize(j);
    if (j.contains("ParticleData") && particleObj_) {
        particleObj_->Deserialize(j["ParticleData"]);
    }
}
