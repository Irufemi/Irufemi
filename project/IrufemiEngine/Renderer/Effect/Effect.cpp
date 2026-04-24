#include "Effect.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"
#include "Engine/Manager/DebugUI.h"
#include "Resource/Texture/TextureManager.h"

Effect::Effect() = default;
Effect::~Effect() = default;

void Effect::Initialize(Camera* camera, EffectType type) {
    camera_ = camera;
    type_ = type;
    gpuParticleSystem_ = std::make_unique<GPUParticleSystem>();
    
    switch (type_) {
    case EffectType::kHit:
        // 現在設定されているテクスチャと形状で初期化
        gpuParticleSystem_->Initialize(camera, currentTextureName_);
        gpuParticleSystem_->SetPrimitive(currentShape_);
        
        // 加算合成で発光表現
        gpuParticleSystem_->SetBlend(BlendMode::kBlendModeAdd);
        // 常にカメラを向くようにビルボードをON
        gpuParticleSystem_->SetBillboard(true);
        
        // HitEffectの設定を適用
        gpuParticleSystem_->SetColor(hitConfig_.color);
        gpuParticleSystem_->SetParticleLife(hitConfig_.lifeMin, hitConfig_.lifeMax);
        
        gpuParticleSystem_->SetParticleScale(
            hitConfig_.startScaleMin, hitConfig_.startScaleMax,
            hitConfig_.endScaleMin,   hitConfig_.endScaleMax
        );

        // 拡散（Jitter）
        gpuParticleSystem_->SetJitter(hitConfig_.jitter);

        // 光るように加算合成にする
        gpuParticleSystem_->SetBlend(BlendMode::kBlendModeAdd);

        // デフォルトでは放出しない
        gpuParticleSystem_->SetEmit(false);
        // ループさせない
        gpuParticleSystem_->SetLoop(false);
        break;
    }
}

void Effect::Update() {
    if (gpuParticleSystem_) {
        gpuParticleSystem_->Update();
    }
}

void Effect::Draw() {
    if (gpuParticleSystem_) {
        gpuParticleSystem_->Draw();
    }
}

void Effect::Debug(const char* name) {
#if defined(USE_IMGUI)
    if (ImGui::Begin(name)) {
        // 種類の選択（将来追加されるエフェクト用）
        const char* typeNames[] = { "Hit" }; // 新しい種類を追加した場合はここに名前を追加する
        int currentType = static_cast<int>(type_);
        if (ImGui::Combo("Effect Type", &currentType, typeNames, IM_ARRAYSIZE(typeNames))) {
            if (camera_) {
                Initialize(camera_, static_cast<EffectType>(currentType));
            }
        }
        
        ImGui::Separator();
        
        ImGui::Text("Global Effect Settings");
        const char* primitiveShapeNames[] = { "Triangle", "Plane", "Cube", "Cylinder", "Sphere", "Tetra", "Circle", "Ring", "Skybox" };
        int currentShape = static_cast<int>(currentShape_);
        if (ImGui::Combo("Primitive Shape", &currentShape, primitiveShapeNames, IM_ARRAYSIZE(primitiveShapeNames))) {
            currentShape_ = static_cast<PrimitiveType>(currentShape);
            gpuParticleSystem_->SetPrimitive(currentShape_);
        }

        if (auto* tm = GPUParticleSystem::GetTextureManager()) {
            auto textureNames = tm->GetTextureNamesForDebug();
            if (ImGui::BeginCombo("Texture", currentTextureName_.c_str())) {
                for (size_t i = 0; i < textureNames.size(); i++) {
                    bool is_selected = (currentTextureName_ == textureNames[i]);
                    if (ImGui::Selectable(textureNames[i].c_str(), is_selected)) {
                        currentTextureName_ = textureNames[i];
                        gpuParticleSystem_->SetTexture(currentTextureName_);
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
        
        ImGui::Separator();
        
        if (ImGui::BeginTabBar("EffectParamsTabs")) {
            if (ImGui::BeginTabItem("Hit Effect Config")) {
                bool changed = false;

                if (ImGui::ColorEdit4("Color", &hitConfig_.color.x)) changed = true;
                if (ImGui::DragFloat2("Life (Min/Max)", &hitConfig_.lifeMin, 0.01f, 0.01f, 10.0f)) changed = true;
                if (ImGui::DragFloat("Jitter", &hitConfig_.jitter, 0.001f, 0.0f, 1.0f)) changed = true;
                if (ImGui::DragInt("Emit Count", &hitConfig_.emitCount, 1, 1, 500)) changed = true;

                ImGui::Separator();
                ImGui::Text("Scale Parameters");
                if (ImGui::DragFloat3("Start Scale Min", &hitConfig_.startScaleMin.x, 0.01f)) changed = true;
                if (ImGui::DragFloat3("Start Scale Max", &hitConfig_.startScaleMax.x, 0.01f)) changed = true;
                if (ImGui::DragFloat3("End Scale Min", &hitConfig_.endScaleMin.x, 0.01f)) changed = true;
                if (ImGui::DragFloat3("End Scale Max", &hitConfig_.endScaleMax.x, 0.01f)) changed = true;

                if (changed && type_ == EffectType::kHit) {
                    // 即座に反映
                    gpuParticleSystem_->SetColor(hitConfig_.color);
                    gpuParticleSystem_->SetParticleLife(hitConfig_.lifeMin, hitConfig_.lifeMax);
                    gpuParticleSystem_->SetJitter(hitConfig_.jitter);
                    gpuParticleSystem_->SetParticleScale(
                        hitConfig_.startScaleMin, hitConfig_.startScaleMax,
                        hitConfig_.endScaleMin, hitConfig_.endScaleMax
                    );
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Separator();

        // テスト用の発生位置を指定して再生するUI
        static Vector3 testPosition = {0.0f, 0.0f, 0.0f};
        ImGui::DragFloat3("Test Position", &testPosition.x, 0.1f);
        
        if (ImGui::Button("Play Effect")) {
            Play(testPosition);
        }
    }
    ImGui::End();
#endif
}

void Effect::Play(const Vector3& position) {
    OutputDebugStringA("[Effect::Play] Called!\n");
    if (!gpuParticleSystem_) return;
    
    switch (type_) {
    case EffectType::kHit:
        gpuParticleSystem_->SetSphereEmitter(position, 0.0f, hitConfig_.emitCount, 0.0f);
        gpuParticleSystem_->SetVelocity(0.0f); // 中心にとどまる
        gpuParticleSystem_->Emit(hitConfig_.emitCount);
        break;
    }
}
