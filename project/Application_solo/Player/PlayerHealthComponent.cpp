#include "Player/PlayerHealthComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/Collider/ColliderComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Core/Utility/Log.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

void PlayerHealthComponent::LoadStatusFromJson() {
    if (statusDataPath_.empty()) {
        return;
    }

    std::ifstream file(statusDataPath_);
    if (!file.is_open()) {
        Log::OutPutLog(std::cout, "[PlayerHealth] Failed to load status: " + statusDataPath_ + "\n");
        return;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("maxHp")) {
            maxHp_ = j["maxHp"].get<int>();
            hp_ = maxHp_;
        }
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
    onDamageTakenListeners_.clear();
    onPlayerDiedListeners_.clear();
}

void PlayerHealthComponent::Start() {
    if (gameObject_) {
        collider_ = gameObject_->GetComponentByInterface<ColliderComponent>();
        if (collider_) {
            collider_->SetDebugCategory(DebugCategory::Combat);
            collider_->SetDebugCustomColor(Irufemi::Vector4{0.0f, 1.0f, 1.0f, 1.0f});
        }
    }
}

void PlayerHealthComponent::Update() {
#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
    if (BaseModel::GetIrufemiEngine()->GetInputManager()->IsKeyPressed(VK_F9)) {
        isGodMode_ = !isGodMode_;
        Log::OutPutLog(std::cout, std::string("[PlayerHealth] God Mode ") + (isGodMode_ ? "ON\n" : "OFF\n"));
    }
#endif

    if (collider_) {
        if (IsInvincible()) {
            collider_->SetDebugCustomColor(Irufemi::Vector4{1.0f, 1.0f, 1.0f, 1.0f});
        } else {
            collider_->SetDebugCustomColor(Irufemi::Vector4{0.0f, 1.0f, 1.0f, 1.0f});
        }
    }

    if (isDead_) {
        if (!hasTriggeredDeathSequenceFinished_) {
            float currentTime = BaseModel::GetIrufemiEngine()->GetGameTime();
            if (currentTime >= deathStartTime_ + 3.0f) {
                hasTriggeredDeathSequenceFinished_ = true;
                if (onDeathSequenceFinished) {
                    onDeathSequenceFinished();
                }
            }
        }
        return;
    }

    float dt = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (dt <= 0.0f) {
        return;
    }

    // 無敵タイマーの更新
    if (invincibilityTimer_ > 0.0f) {
        invincibilityTimer_ -= dt;
        if (invincibilityTimer_ < 0.0f) {
            invincibilityTimer_ = 0.0f;
        }
    }
}

void PlayerHealthComponent::TakeDamage(int damage) {
    if (isDead_) {
        return;
    }

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

        if (onPlayerDied) {
            onPlayerDied();
        }
        for (const auto& listener : onPlayerDiedListeners_) {
            if (listener) {
                listener();
            }
        }

        Log::OutPutLog(std::cout, "[PlayerHealth] Player Died!\n");
        return;
    }

    invincibilityTimer_ = maxInvincibilityTime_;

    for (const auto& listener : onDamageTakenListeners_) {
        if (listener) {
            listener(damage);
        }
    }
}
