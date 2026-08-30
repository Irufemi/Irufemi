#include "Player/PlayerHealthComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/SkinnedMeshRendererComponent.h"
#include "Framework/Component/Effect/ScreenEffectComponent.h"
#include "Framework/Component/Camera/CameraShakeComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Core/System/IrufemiEngine.h"
#include "Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Core/Utility/Log.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cmath>

void PlayerHealthComponent::LoadStatusFromJson() {
    if (statusDataPath_.empty()) return;

    std::ifstream file(statusDataPath_);
    if (!file.is_open()) {
        Log::OutPutLog(std::cout, "[PlayerHealth] Failed to load status: " + statusDataPath_ + "\n");
        return;
    }

    try {
        nlohmann::json j;
        file >> j;
        
        if (j.contains("maxHp")) { maxHp_ = j["maxHp"].get<int>(); hp_ = maxHp_; }
    } catch (const std::exception& e) {
        Log::OutPutLog(std::cout, std::string("[PlayerHealth] JSON Parse Error: ") + e.what() + "\n");
    }
}

void PlayerHealthComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Status Data Path", &statusDataPath_);
    RegisterProperty("God Mode", &isGodMode_);
}

void PlayerHealthComponent::Initialize() {
    LoadStatusFromJson();
    
    invincibilityTimer_ = 0.0f;
    flashTimer_ = 0.0f;
    flashInterval_ = 0.1f;
    colorCached_ = false;
}

void PlayerHealthComponent::Start() {
}

void PlayerHealthComponent::Update() {
#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
    if (BaseModel::GetIrufemiEngine()->GetInputManager()->IsKeyPressed(VK_F9)) {
        isGodMode_ = !isGodMode_;
        Log::OutPutLog(std::cout, std::string("[PlayerHealth] God Mode ") + (isGodMode_ ? "ON\n" : "OFF\n"));
    }
#endif

    if (isDead_) {
        if (!hasTriggeredDeathSequenceFinished_) {
            float currentTime = BaseModel::GetIrufemiEngine()->GetGameTime();
            if (currentTime >= deathStartTime_ + 3.0f) {
                hasTriggeredDeathSequenceFinished_ = true;
                if (onDeathSequenceFinished) onDeathSequenceFinished();
            }
        }
        return;
    }

    float dt = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (dt <= 0.0f) return;

    // --- 被弾時の無敵時間と点滅処理 ---
    if (invincibilityTimer_ > 0.0f) {
        invincibilityTimer_ -= dt;
        flashTimer_ += dt;
        
        BaseModel* model = nullptr;
        if (auto mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
            model = reinterpret_cast<BaseModel*>(mesh->GetRenderable());
        } else if (auto skinned = gameObject_->GetComponent<SkinnedMeshRendererComponent>()) {
            model = reinterpret_cast<BaseModel*>(skinned->GetRenderable());
        }
        
        if (model) {
            if (!colorCached_) {
                originalBaseColor_ = model->GetColor();
                colorCached_ = true;
            }
            if (fmod(flashTimer_, flashInterval_ * 2.0f) < flashInterval_) {
                model->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 赤色
            } else {
                model->SetColor(originalBaseColor_); // 通常色
            }
        }
        
        if (invincibilityTimer_ <= 0.0f) {
            if (model && colorCached_) {
                model->SetColor(originalBaseColor_);
            }
        }
    }
}

void PlayerHealthComponent::TakeDamage(int damage) {
    if (isDead_) return;
    
    if (isGodMode_) {
        Log::OutPutLog(std::cout, "[PlayerHealth] TakeDamage ignored (God Mode)\n");
        return;
    }

    if (IsInvincible()) {
        Log::OutPutLog(std::cout, "[PlayerHealth] TakeDamage ignored (Invincible)\n");
        return;
    }

    hp_ -= damage;
    Log::OutPutLog(std::cout, "[PlayerHealth] Took Damage! HP: " + std::to_string(hp_) + "\n");
    if (hp_ <= 0) {
        hp_ = 0;
        isDead_ = true;
        deathStartTime_ = BaseModel::GetIrufemiEngine()->GetGameTime();
        if (onPlayerDied) onPlayerDied();
        Log::OutPutLog(std::cout, "[PlayerHealth] Player Died!\n");
        
        // 自機が死んだときに自機のモデルの描画を切る
        if (auto mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
            mesh->SetVisible(false);
        } else if (auto skinned = gameObject_->GetComponent<SkinnedMeshRendererComponent>()) {
            skinned->SetVisible(false);
        }
        
        return;
    }

    Log::OutPutLog(std::cout, "[PlayerHealth] Triggering flashing...\n");
    invincibilityTimer_ = maxInvincibilityTime_;
    isFlashing_ = true;
    flashTimer_ = 0.0f;
    
    // カメラシェイク発火 (プレイヤー被弾時なので強め)
    if (auto scene = gameObject_->GetScene()) {
        if (auto mainCameraObj = scene->FindGameObject("MainCamera")) {
            if (auto shakeComp = mainCameraObj->GetComponent<CameraShakeComponent>()) {
                shakeComp->PlayShake(1.0f, 30, 20.0f); // Intensity=1.0, 30 Frames, Freq=20
            }
        }
    }

    // ポストエフェクト演出の再生
    auto& comps = gameObject_->GetComponents();
    for (auto& comp : comps) {
        if (auto screenEffect = std::dynamic_pointer_cast<ScreenEffectComponent>(comp)) {
            screenEffect->Play();
        }
    }
}
