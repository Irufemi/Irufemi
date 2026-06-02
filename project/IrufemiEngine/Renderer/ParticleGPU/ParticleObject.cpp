#include "ParticleObject.h"
#include "Renderer/ParticleGPU/GPUParticleManager.h"

TextureManager* ParticleObject::textureManager_ = nullptr;

ParticleObject::ParticleObject() {}

ParticleObject::~ParticleObject() {
    if (emitterHandle_.IsValid()) {
        GPUParticleManager::GetInstance()->UnregisterEmitter(emitterHandle_);
    }
}

void ParticleObject::Initialize() {
    if (emitOnAwake_) {
        Play();
    }
}

void ParticleObject::Play() {
    isPlaying_ = true;
    if (!emitterHandle_.IsValid()) {
        emitterHandle_ = GPUParticleManager::GetInstance()->RegisterEmitter(texturePath_);
    }
}

void ParticleObject::Stop() {
    isPlaying_ = false;
}

void ParticleObject::EmitBurst(int count) {
    burstCountPending_ += count;
    Play();
}

void ParticleObject::Update() {
    // Stop直後にも emit=0 を送信させるため、ここで早期リターンはしない
    if (!emitterHandle_.IsValid()) {
        emitterHandle_ = GPUParticleManager::GetInstance()->RegisterEmitter(texturePath_);
    }

    GPUParticleEmitter data;
    data.emit = isPlaying_ ? 1 : 0;
    
    data.type = emitType_;
    data.translateX = position_.x;
    data.translateY = position_.y;
    data.translateZ = position_.z;
    data.count = emitCountPerFrame_;
    data.frequency = emitFrequency_;
    data.minLife = lifeTimeMin_;
    data.maxLife = lifeTimeMax_;
    data.velocity = velocity_;
    data.radius = radius_;
    data.spread = spread_;
    data.gravity = gravity_;
    data.damping = damping_;

    data.startColorMinR = color_.x; data.startColorMinG = color_.y; data.startColorMinB = color_.z; data.startColorMinA = color_.w;
    data.startColorMaxR = color_.x; data.startColorMaxG = color_.y; data.startColorMaxB = color_.z; data.startColorMaxA = color_.w;
    
    data.midColorMinR = midColor_.x; data.midColorMinG = midColor_.y; data.midColorMinB = midColor_.z; data.midColorMinA = midColor_.w;
    data.midColorMaxR = midColor_.x; data.midColorMaxG = midColor_.y; data.midColorMaxB = midColor_.z; data.midColorMaxA = midColor_.w;

    data.endColorMinR = color_.x; data.endColorMinG = color_.y; data.endColorMinB = color_.z; data.endColorMinA = 0.0f;
    data.endColorMaxR = color_.x; data.endColorMaxG = color_.y; data.endColorMaxB = color_.z; data.endColorMaxA = 0.0f;

    data.startScaleMinX = startScale_.x; data.startScaleMinY = startScale_.y; data.startScaleMinZ = startScale_.z;
    data.startScaleMaxX = startScale_.x; data.startScaleMaxY = startScale_.y; data.startScaleMaxZ = startScale_.z;
    
    data.midScaleMinX = midScale_.x; data.midScaleMinY = midScale_.y; data.midScaleMinZ = midScale_.z;
    data.midScaleMaxX = midScale_.x; data.midScaleMaxY = midScale_.y; data.midScaleMaxZ = midScale_.z;

    data.endScaleMinX = endScale_.x; data.endScaleMinY = endScale_.y; data.endScaleMinZ = endScale_.z;
    data.endScaleMaxX = endScale_.x; data.endScaleMaxY = endScale_.y; data.endScaleMaxZ = endScale_.z;
    
    data.midPoint = midPoint_;

    data.directionX = direction_.x;
    data.directionY = direction_.y;
    data.directionZ = direction_.z;
    
    data.areaSizeX = areaSize_.x;
    data.areaSizeY = areaSize_.y;
    data.areaSizeZ = areaSize_.z;
    
    data.bounce = bounce_;
    data.groundHeight = groundHeight_;
    
    data.attractorStrength = attractorStrength_;
    data.attractorPosX = attractorPos_.x;
    data.attractorPosY = attractorPos_.y;
    data.attractorPosZ = attractorPos_.z;
    
    data.atlasRows = atlasRows_;
    data.atlasCols = atlasCols_;
    data.billboardMode = billboardMode_;
    data.jitter = jitter_;

    if (burstCountPending_ > 0) {
        data.burstCount = burstCountPending_;
        burstCountPending_ = 0;
    }
    GPUParticleManager::GetInstance()->UpdateEmitterData(emitterHandle_, data);
}
#ifdef USE_IMGUI
#include <imgui.h>
#endif
#include "Resource/Texture/TextureManager.h"

void ParticleObject::DebugUI(const char* name) {
#ifdef USE_IMGUI
    if (ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::TreeNodeEx("General", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Texture Manager Combo
            auto* textureManager = GetTextureManager();
            if (textureManager && !textureManager->GetTextureNamesForDebug().empty()) {
                auto textureNames = textureManager->GetTextureNamesForDebug();
                std::vector<const char*> namesCStr;
                int currentIndex = -1;
                for (int i = 0; i < textureNames.size(); ++i) {
                    namesCStr.push_back(textureNames[i].c_str());
                    if (texturePath_ == textureNames[i]) {
                        currentIndex = i;
                    }
                }
                if (currentIndex == -1) currentIndex = 0; // fallback
                
                if (ImGui::Combo("Texture", &currentIndex, namesCStr.data(), (int)namesCStr.size())) {
                    if (texturePath_ != textureNames[currentIndex]) {
                        texturePath_ = textureNames[currentIndex];
                        if (emitterHandle_.IsValid()) {
                            GPUParticleManager::GetInstance()->UnregisterEmitter(emitterHandle_);
                            emitterHandle_ = GPUParticleManager::GetInstance()->RegisterEmitter(texturePath_);
                        }
                    }
                }
            } else {
                char texBuffer[256];
                strncpy_s(texBuffer, texturePath_.c_str(), sizeof(texBuffer));
                if (ImGui::InputText("Texture Path", texBuffer, sizeof(texBuffer))) {
                    if (texturePath_ != texBuffer) {
                        texturePath_ = texBuffer;
                        if (emitterHandle_.IsValid()) {
                            GPUParticleManager::GetInstance()->UnregisterEmitter(emitterHandle_);
                            emitterHandle_ = GPUParticleManager::GetInstance()->RegisterEmitter(texturePath_);
                        }
                    }
                }
            }
            
            // Emit On Awake
            ImGui::Checkbox("Emit On Awake", &emitOnAwake_);

            ImGui::Separator();
            ImGui::DragInt("Atlas Rows", &atlasRows_, 1, 1, 16);
            ImGui::DragInt("Atlas Cols", &atlasCols_, 1, 1, 16);
            
            ImGui::Separator();
            if (ImGui::Button("Test Burst (50 particles)")) {
                EmitBurst(50);
            }
            ImGui::SameLine();
            if (ImGui::Button("Play")) Play();
            ImGui::SameLine();
            if (ImGui::Button("Stop")) Stop();

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Shape & Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* presetNames[] = { "Custom", "Explosion", "Spark", "Smoke" };
            if (ImGui::Combo("Preset", &particleType_, presetNames, 4)) {
                if (particleType_ == 1) { // Explosion
                    emitType_ = 0; // Sphere
                    velocity_ = 5.0f;
                    gravity_ = 0.0f;
                    damping_ = 0.05f;
                    lifeTimeMin_ = 0.3f;
                    lifeTimeMax_ = 0.6f;
                } else if (particleType_ == 2) { // Spark
                    emitType_ = 0;
                    velocity_ = 3.0f;
                    gravity_ = -9.8f;
                    damping_ = 0.0f;
                    bounce_ = 0.6f;
                    groundHeight_ = 0.0f;
                } else if (particleType_ == 3) { // Smoke
                    emitType_ = 0;
                    velocity_ = 1.0f;
                    gravity_ = 1.0f;
                    damping_ = 0.02f;
                }
            }
            const char* shapeNames[] = { "Sphere", "Beam", "Box", "Cylinder" };
            if (ImGui::Combo("Emit Shape", &emitType_, shapeNames, 4)) {}

            const char* billboardNames[] = { "None", "Billboard", "Y-Axis" };
            if (ImGui::Combo("Billboard Mode", &billboardMode_, billboardNames, 3)) {}

            ImGui::Separator();
            if (emitType_ == 2) { // Box
                ImGui::DragFloat3("Area Size", &areaSize_.x, 0.1f, 0.0f, 100.0f);
            } else {
                ImGui::DragFloat("Radius / Size", &radius_, 0.1f, 0.0f, 100.0f);
            }
            
            ImGui::DragFloat("Spread", &spread_, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat3("Direction", &direction_.x, 0.05f);

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Emission Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragInt("Emit Count/Frame", &emitCountPerFrame_, 1, 0, 1000);
            ImGui::DragFloat("Frequency", &emitFrequency_, 0.01f, 0.0f, 10.0f);
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Physics & Kinetics", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat("Velocity", &velocity_, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Gravity", &gravity_, 0.1f, -50.0f, 50.0f);
            ImGui::DragFloat("Damping", &damping_, 0.005f, 0.0f, 1.0f);
            ImGui::DragFloat("Bounce", &bounce_, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Ground Height", &groundHeight_, 0.1f, -100.0f, 100.0f);
            
            ImGui::Separator();
            ImGui::DragFloat("Attractor Strength", &attractorStrength_, 0.1f, -50.0f, 50.0f);
            ImGui::DragFloat3("Attractor Pos", &attractorPos_.x, 0.1f);

            ImGui::Separator();
            ImGui::DragFloat("Jitter", &jitter_, 0.01f, 0.0f, 10.0f);

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Lifetime & Visuals", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat("Life Min", &lifeTimeMin_, 0.05f, 0.01f, 10.0f);
            ImGui::DragFloat("Life Max", &lifeTimeMax_, 0.05f, 0.01f, 10.0f);
            
            ImGui::Separator();
            ImGui::ColorEdit4("Start Color", &color_.x);
            ImGui::ColorEdit4("Mid Color", &midColor_.x);
            
            ImGui::Separator();
            ImGui::DragFloat3("Start Scale", &startScale_.x, 0.05f);
            ImGui::DragFloat3("Mid Scale", &midScale_.x, 0.05f);
            ImGui::DragFloat3("End Scale", &endScale_.x, 0.05f);

            ImGui::Separator();
            ImGui::SliderFloat("Mid Point (0~1)", &midPoint_, 0.0f, 1.0f);

            ImGui::TreePop();
        }
    }
#endif
}
