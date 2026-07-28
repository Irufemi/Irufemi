#pragma once
#include "../Component.h"
#include "Renderer/System/ParticleGPU/GPUParticleManager.h"

class TransformComponent;

class ParticleFieldComponent : public Component {
public:
    ParticleFieldComponent();
    ~ParticleFieldComponent() override;

    void Initialize() override;
    void Update() override;
    void Draw() override {}
    
    IRenderable* GetRenderable() override { return nullptr; }
    bool CanUpdateInEditMode() const override { return true; }

    std::string GetComponentName() const override { return "ParticleFieldComponent"; }
    void OnRegisterProperties() override;

    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    ParticleField& GetFieldData() { return fieldData_; }

private:
    TransformComponent* transform_ = nullptr;
    GPUParticleManager::FieldHandle fieldHandle_;
    
    ParticleField fieldData_;
};
