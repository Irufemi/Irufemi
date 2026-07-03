#include "GravityPlayerComponent.h"
#include "PlayerTargetingComponent.h"
#include "DebrisComponent.h"
#include "DebrisManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Engine/Platform/Input/Mouse.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Engine/Core/Math/Random/Random.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Core/Math/MathFunction.h"
#include "BossComponent.h"
#include <algorithm>

void GravityPlayerComponent::OnRegisterProperties() {
    RegisterProperty("Max Orbit Count", &maxOrbitCount_);
    RegisterProperty("Pull Radius", &pullRadius_);
    RegisterProperty("Throw Interval", &throwInterval_);
}

void GravityPlayerComponent::Initialize() {
    orbitingDebris_.clear();
    isThrowing_ = false;
    throwTimer_ = 0.0f;
}

void GravityPlayerComponent::Start() {
    if (!gameObject_) return;
    targetingComp_ = gameObject_->GetComponent<PlayerTargetingComponent>();
    
    auto scene = gameObject_->GetScene();
    if (scene) {
        auto debrisManagerObj = scene->FindGameObject("DebrisManager");
        if (debrisManagerObj) {
            debrisManager_ = debrisManagerObj->GetComponent<DebrisManagerComponent>();
        }
    }
}

void GravityPlayerComponent::Update() {
    // 無効になったガレキを除外
    orbitingDebris_.erase(
        std::remove_if(orbitingDebris_.begin(), orbitingDebris_.end(),
            [](const std::shared_ptr<GameObject>& obj) {
                if (!obj || !obj->GetIsActive()) return true;
                auto comp = obj->GetComponent<DebrisComponent>();
                return !comp || (comp->GetState() != DebrisState::Orbiting && comp->GetState() != DebrisState::Pulled);
            }),
        orbitingDebris_.end()
    );

    if (isThrowing_) {
        UpdateThrowing();
    } else {
        if (targetingComp_) {
            size_t maxLockOn = orbitingDebris_.size();
            if (maxLockOn == 0) maxLockOn = 1;
            targetingComp_->SetMaxLockonCount(maxLockOn);
        }
        HandlePullInput();
        HandleMarkInput();
        HandleThrowInput();
    }
}

void GravityPlayerComponent::HandlePullInput() {
    auto input = BaseModel::GetIrufemiEngine()->GetInputManager();
    if (!input) return;

    // Eキー で引き寄せ (右クリックは廃止)
    if (input->IsKeyPressed('E')) {
        if (static_cast<int>(orbitingDebris_.size()) >= maxOrbitCount_) return;

        auto scene = gameObject_->GetScene();
        if (!scene) return;

        auto transform = gameObject_->GetComponent<TransformComponent>();
        if (!transform) return;

        // ボスのシールドを奪う処理
        if (targetingComp_) {
            auto& targets = targetingComp_->GetQueuedTargets();
            for (auto& target : targets) {
                if (!target) continue;
                if (auto debrisComp = target->GetComponent<DebrisComponent>()) {
                    if (debrisComp->GetState() == DebrisState::BossOrbiting) {
                        if (auto bossTarget = debrisComp->GetTarget().lock()) {
                            if (auto bossComp = bossTarget->GetComponent<BossComponent>()) {
                                bossComp->RemoveShield(target);
                            }
                        }
                        debrisComp->SetState(DebrisState::Pulled);
                        debrisComp->SetTarget(gameObject_->shared_from_this());
                        debrisComp->SetOrbitParams(Random::GeneratorFloat(0.0f, 6.28f), Random::GeneratorFloat(2.0f, 4.0f));
                        orbitingDebris_.push_back(target);
                        
                        // 一つ奪ったらキューをクリアして終了
                        targetingComp_->ClearTargets();
                        return;
                    }
                }
            }
        }

        if (!debrisManager_) return;

        auto debrisObj = debrisManager_->ExtractNearestIdleDebris(transform->GetPosition(), pullRadius_);
        if (debrisObj) {
            if (auto debrisComp = debrisObj->GetComponent<DebrisComponent>()) {
                debrisComp->SetState(DebrisState::Pulled);
                debrisComp->SetTarget(gameObject_->shared_from_this());
                debrisComp->SetOrbitParams(Random::GeneratorFloat(0.0f, 6.28f), Random::GeneratorFloat(2.0f, 4.0f));
                orbitingDebris_.push_back(debrisObj);
            }
        }
    }
}

void GravityPlayerComponent::HandleMarkInput() {
    if (!targetingComp_) return;
    auto input = BaseModel::GetIrufemiEngine()->GetInputManager();
    if (!input) return;

    // Rキーでキャンセル
    if (input->IsKeyDown('R')) {
        targetingComp_->ClearTargets();
    }

    // 右クリックでマーキング
    if (input->IsMouseButtonDown(Mouse::Button::Right)) {
        size_t maxLockOn = orbitingDebris_.size();
        if (maxLockOn == 0) maxLockOn = 1; // シールド奪取用に最低1つはロック許可
        targetingComp_->MarkTarget(maxLockOn);
    }
}

void GravityPlayerComponent::HandleThrowInput() {
    auto input = BaseModel::GetIrufemiEngine()->GetInputManager();
    if (!input) return;

    // 左クリックで一斉掃射モードへ
    if (input->IsMouseButtonDown(Mouse::Button::Left) || input->IsKeyPressed('Q')) {
        if (orbitingDebris_.empty()) return;
        isThrowing_ = true;
        throwTimer_ = throwInterval_; // 即座に1発目を撃つため
    }
}

void GravityPlayerComponent::UpdateThrowing() {
    if (orbitingDebris_.empty()) {
        isThrowing_ = false;
        if (targetingComp_) targetingComp_->ClearTargets(); // 弾切れで残ったマークをクリア
        return;
    }

    auto engine = BaseModel::GetIrufemiEngine();
    throwTimer_ += engine->GetDeltaTime();

    if (throwTimer_ >= throwInterval_) {
        throwTimer_ = 0.0f;

        auto debris = orbitingDebris_.back();
        orbitingDebris_.pop_back();

        if (debris) {
            auto comp = debris->GetComponent<DebrisComponent>();
            if (comp) {
                std::shared_ptr<GameObject> throwTarget = nullptr;
                if (targetingComp_) {
                    throwTarget = targetingComp_->PopTarget();
                }

                comp->SetState(DebrisState::Thrown);
                comp->SetTarget(throwTarget);

                if (!throwTarget) {
                    auto cameraManager = engine->GetCameraManager();
                    if (cameraManager && cameraManager->GetActiveCamera()) {
                        auto camera = cameraManager->GetActiveCamera();
                        Matrix4x4 viewMat = camera->GetViewMatrix();
                        Vector3 forward = { viewMat.m[0][2], viewMat.m[1][2], viewMat.m[2][2] };
                        comp->SetThrowDirection({ forward.x, forward.y, forward.z });
                    }
                }
            }
        }
    }
}
