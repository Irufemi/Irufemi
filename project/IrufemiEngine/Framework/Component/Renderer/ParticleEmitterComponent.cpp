#include "ParticleEmitterComponent.h"
#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"

ParticleEmitterComponent::ParticleEmitterComponent() {}

ParticleEmitterComponent::~ParticleEmitterComponent() {
    GPUParticleManager::GetInstance()->UnregisterEmitter(emitterHandle_);
}

void ParticleEmitterComponent::OnRegisterProperties() {
    RegisterHeader("General");
    RegisterProperty("Texture Path", &texturePath_);
    RegisterProperty("Emit On Awake", &emitOnAwake_);
    
    RegisterSeparator();
    RegisterHeader("Presets & Type");
    RegisterProperty("Particle Type (0:Cst,1:Exp,2:Spk,3:Smk)", &particleType_);
    RegisterProperty("Emit Type (0-3)", &emitType_);
    
    RegisterSeparator();
    RegisterHeader("Emission Parameters");
    RegisterProperty("Emit Count", &emitCountPerFrame_);
    RegisterProperty("Frequency", &emitFrequency_);
    RegisterProperty("Velocity", &velocity_);
    RegisterProperty("Radius", &radius_);
    RegisterProperty("Spread", &spread_);
    RegisterProperty("Direction", &direction_);
    
    RegisterSeparator();
    RegisterHeader("Physics");
    RegisterProperty("Gravity", &gravity_);
    RegisterProperty("Damping", &damping_);
    
    RegisterSeparator();
    RegisterHeader("Lifetime & Visuals");
    RegisterProperty("Life Min", &lifeTimeMin_);
    RegisterProperty("Life Max", &lifeTimeMax_);
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

    // プリセットによるパラメータの上書き
    if (particleType_ == 1) { // Explosion
        emitType_ = 0; // Sphere
        gravity_ = 0.0f;
        damping_ = 0.05f;
        velocity_ = 8.0f;
        lifeTimeMin_ = 0.5f;
        lifeTimeMax_ = 1.0f;
    } else if (particleType_ == 2) { // Spark
        emitType_ = 0; // Sphere
        gravity_ = -9.8f;
        damping_ = 0.02f;
        velocity_ = 5.0f;
        lifeTimeMin_ = 1.0f;
        lifeTimeMax_ = 2.0f;
    } else if (particleType_ == 3) { // Smoke
        emitType_ = 0; // Sphere
        gravity_ = 2.0f; // 上へ
        damping_ = 0.1f;
        velocity_ = 1.0f;
        lifeTimeMin_ = 2.0f;
        lifeTimeMax_ = 3.0f;
    }

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
    
    // Physics
    data.gravity = gravity_;
    data.damping = damping_;
    
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
    
    // Burst
    data.burstCount = burstCountPending_;
    burstCountPending_ = 0; // 送信後はリセット
    
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

void ParticleEmitterComponent::EmitBurst(int count) {
    burstCountPending_ = count;
}
