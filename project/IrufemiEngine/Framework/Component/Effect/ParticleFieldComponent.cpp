#include "ParticleFieldComponent.h"
#include "../TransformComponent.h"
#include "../../GameObject.h"
#include "../../BaseScene.h"
#include "../../../Engine/IrufemiEngine.h"
ParticleFieldComponent::ParticleFieldComponent() {
    fieldData_.type = 1; // Default to Point Attractor
    fieldData_.strength = 10.0f;
    fieldData_.range = 50.0f;
    fieldData_.falloff = 1.0f;
    fieldData_.direction = {0.0f, -1.0f, 0.0f};
    fieldData_.axis = {0.0f, 1.0f, 0.0f};
}

ParticleFieldComponent::~ParticleFieldComponent() {
    if (fieldHandle_.IsValid() && GetGameObject() && GetGameObject()->GetScene() && GetGameObject()->GetScene()->GetEngine()->GetGPUParticleManager()) {
        GetGameObject()->GetScene()->GetEngine()->GetGPUParticleManager()->UnregisterField(fieldHandle_);
    }
}

void ParticleFieldComponent::Initialize() {
    if (GetGameObject() && GetGameObject()->GetScene() && GetGameObject()->GetScene()->GetEngine()->GetGPUParticleManager()) {
        fieldHandle_ = GetGameObject()->GetScene()->GetEngine()->GetGPUParticleManager()->RegisterField();
    }
}

void ParticleFieldComponent::Update() {
    if (GetTransform()) {
        fieldData_.position = GetTransform()->GetWorldPosition();
    }
    
    if (fieldHandle_.IsValid() && GetGameObject() && GetGameObject()->GetScene() && GetGameObject()->GetScene()->GetEngine()->GetGPUParticleManager()) {
        GetGameObject()->GetScene()->GetEngine()->GetGPUParticleManager()->UpdateFieldData(fieldHandle_, fieldData_);
    }
}

void ParticleFieldComponent::OnRegisterProperties() {
    RegisterProperty("Field Type", (int*)&fieldData_.type)
        .SetTooltip("0: Directional, 1: Point Attractor, 2: Vortex");
    RegisterProperty("Strength", &fieldData_.strength).SetMinMax(-1000.0f, 1000.0f);
    RegisterProperty("Effect Range", &fieldData_.range).SetMinMax(0.0f, 10000.0f);
    RegisterProperty("Falloff", &fieldData_.falloff).SetMinMax(0.0f, 10.0f);
    RegisterProperty("Direction X", &fieldData_.direction.x).SetMinMax(-1.0f, 1.0f);
    RegisterProperty("Direction Y", &fieldData_.direction.y).SetMinMax(-1.0f, 1.0f);
    RegisterProperty("Direction Z", &fieldData_.direction.z).SetMinMax(-1.0f, 1.0f);
    RegisterProperty("Axis X", &fieldData_.axis.x).SetMinMax(-1.0f, 1.0f);
    RegisterProperty("Axis Y", &fieldData_.axis.y).SetMinMax(-1.0f, 1.0f);
    RegisterProperty("Axis Z", &fieldData_.axis.z).SetMinMax(-1.0f, 1.0f);
}

nlohmann::json ParticleFieldComponent::Serialize() {
    nlohmann::json j;
    j["type"] = fieldData_.type;
    j["strength"] = fieldData_.strength;
    j["range"] = fieldData_.range;
    j["falloff"] = fieldData_.falloff;
    j["direction"] = {fieldData_.direction.x, fieldData_.direction.y, fieldData_.direction.z};
    j["axis"] = {fieldData_.axis.x, fieldData_.axis.y, fieldData_.axis.z};
    return j;
}

void ParticleFieldComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("type")) fieldData_.type = j["type"];
    if (j.contains("strength")) fieldData_.strength = j["strength"];
    if (j.contains("range")) fieldData_.range = j["range"];
    if (j.contains("falloff")) fieldData_.falloff = j["falloff"];
    if (j.contains("direction")) {
        fieldData_.direction.x = j["direction"][0];
        fieldData_.direction.y = j["direction"][1];
        fieldData_.direction.z = j["direction"][2];
    }
    if (j.contains("axis")) {
        fieldData_.axis.x = j["axis"][0];
        fieldData_.axis.y = j["axis"][1];
        fieldData_.axis.z = j["axis"][2];
    }
}
