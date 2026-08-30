#include "Framework/Component/Effect/ParticleEmitterComponent.h"
#include "Core/Utility/Log.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject/GameObject.h"
#include <iostream>

ParticleEmitterComponent::ParticleEmitterComponent() {
    particleObj_ = std::make_unique<ParticleObject>();
}

ParticleEmitterComponent::~ParticleEmitterComponent() {}

void ParticleEmitterComponent::OnRegisterProperties() {
    if (particleObj_) {
        particleObj_->RegisterProperties(this);
    }
}

void ParticleEmitterComponent::Initialize() {
    particleObj_->Initialize();
}

void ParticleEmitterComponent::Update() {
    if (GetTransform()) {
        particleObj_->SetPosition(GetTransform()->GetWorldPosition());
    }

#ifdef _DEBUG
    // エディタからの値変更をリアルタイム反映させるため、常にDirtyフラグを立てる
    particleObj_->MarkDirty();
#endif

    static int frameCounter = 0;
    if (frameCounter++ % 60 == 0) {
#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
        Log::OutPutLog(std::cout, "[ParticleEmitterComponent] Update called. Pos: " +
                                      std::to_string(GetTransform()->GetWorldPosition().x) + "\n");
#endif
    }

    particleObj_->Update();
}

void ParticleEmitterComponent::Draw() {}

void ParticleEmitterComponent::Play() {
    particleObj_->Play();
}

void ParticleEmitterComponent::Restart(bool withChildren) {
    if (particleObj_) {
        // GPU側に放出リクエストを送る前に、最新のワールド座標を即座に反映する
        if (GetTransform()) {
            particleObj_->SetPosition(GetTransform()->GetWorldPosition());
        }

#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
        auto pos = particleObj_->GetPosition();
        Log::OutPutLog(std::cout, "[ParticleEmitterComponent] Restart. WorldPos: " + std::to_string(pos.x) + ", " +
                                      std::to_string(pos.y) + ", " + std::to_string(pos.z) + "\n");
#endif

        particleObj_->Restart();
    }

    if (withChildren && gameObject_) {
        auto emitters = gameObject_->GetComponentsInChildren<ParticleEmitterComponent>();
        for (auto childEmitter : emitters) {
            if (childEmitter != this) {
                childEmitter->Restart(false);
            }
        }
    }
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

std::shared_ptr<Component> ParticleEmitterComponent::Clone() {
    auto clone = std::make_shared<ParticleEmitterComponent>();
    clone->CopyPropertiesFrom(this);
    if (this->particleObj_) {
        nlohmann::json particleJson;
        this->particleObj_->Serialize(particleJson);
        clone->particleObj_->Deserialize(particleJson);
    }
    return clone;
}
