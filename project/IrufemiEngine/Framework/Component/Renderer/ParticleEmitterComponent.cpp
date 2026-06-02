#include "ParticleEmitterComponent.h"
#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"

ParticleEmitterComponent::ParticleEmitterComponent() {}

ParticleEmitterComponent::~ParticleEmitterComponent() {
    GPUParticleManager::GetInstance()->UnregisterEmitter(emitterHandle_);
}

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
    std::string fullPath = "resources/" + texturePath_;
    if (texturePath_.find("resources/") == 0) {
        fullPath = texturePath_;
    }
    emitterHandle_ = GPUParticleManager::GetInstance()->RegisterEmitter(fullPath);

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
    if (!emitterHandle_.IsValid()) return;

    Vector3 pos = transform_ ? transform_->worldPosition_ : Vector3{0, 0, 0};

    GPUParticleEmitter data;
    // Set basic params
    data.type = emitType_;
    data.translateX = pos.x; data.translateY = pos.y; data.translateZ = pos.z;
    data.directionX = direction_.x; data.directionY = direction_.y; data.directionZ = direction_.z;
    data.radius = radius_;
    data.velocity = velocity_;
    data.spread = spread_;
    data.count = emitCountPerFrame_;
    data.frequency = emitFrequency_;
    
    // Default shape specific hacks (like Box areaSize, Cylinder height mapping)
    if (emitType_ == 2) { // Box
        data.areaSizeX = radius_; data.areaSizeY = radius_; data.areaSizeZ = radius_;
    } else if (emitType_ == 3) { // Cylinder
        data.velocity = radius_ * 2.0f; // height as velocity in current impl
    }
    
    // Scaling
    data.startScaleMinX = startScale_.x; data.startScaleMinY = startScale_.y; data.startScaleMinZ = startScale_.z;
    data.startScaleMaxX = startScale_.x; data.startScaleMaxY = startScale_.y; data.startScaleMaxZ = startScale_.z;
    data.endScaleMinX = endScale_.x; data.endScaleMinY = endScale_.y; data.endScaleMinZ = endScale_.z;
    data.endScaleMaxX = endScale_.x; data.endScaleMaxY = endScale_.y; data.endScaleMaxZ = endScale_.z;
    
    // Life
    data.minLife = lifeTimeMin_;
    data.maxLife = lifeTimeMax_;
    
    // Colors
    data.startColorMinR = color_.x; data.startColorMinG = color_.y; data.startColorMinB = color_.z; data.startColorMinA = color_.w;
    data.startColorMaxR = color_.x; data.startColorMaxG = color_.y; data.startColorMaxB = color_.z; data.startColorMaxA = color_.w;
    data.endColorMinR = color_.x; data.endColorMinG = color_.y; data.endColorMinB = color_.z; data.endColorMinA = 0.0f;
    data.endColorMaxR = color_.x; data.endColorMaxG = color_.y; data.endColorMaxB = color_.z; data.endColorMaxA = 0.0f;
    
    data.emit = isPlaying_ ? 1 : 0;
    
    GPUParticleManager::GetInstance()->UpdateEmitterData(emitterHandle_, data);
}

void ParticleEmitterComponent::Draw() {
}

void ParticleEmitterComponent::Play() {
    isPlaying_ = true;
}

void ParticleEmitterComponent::Stop() {
    isPlaying_ = false;
}
