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
#include "Engine/Manager/CollisionManager.h"
#include "Boss/BossComponent.h"
#include <algorithm>

void GravityPlayerComponent::OnRegisterProperties() {
    RegisterProperty("Max Orbit Count", &maxOrbitCount_);
    RegisterProperty("Pull Radius", &pullRadius_);
    RegisterProperty("Throw Interval", &throwInterval_);
    RegisterProperty("NoLock Throw Dist", &noLockThrowDistance_);
    RegisterProperty("Orbit Radius Min", &orbitRadiusMin_);
    RegisterProperty("Orbit Radius Max", &orbitRadiusMax_);
    RegisterProperty("Orbit Angle Max", &orbitAngleRandomMax_);
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

        // 1. ロックオン済み、またはホバー中のターゲットを取得
        std::shared_ptr<GameObject> targetToSteal = nullptr;
        bool isQueuedTarget = false;

        if (targetingComp_) {
            // A. まずは手動ロックオン済みのキューをチェック
            auto& targets = targetingComp_->GetQueuedTargets();
            for (auto& t : targets) {
                if (!t) continue;
                if (auto debrisComp = t->GetComponent<DebrisComponent>()) {
                    if (debrisComp->GetState() == DebrisState::BossOrbiting) {
                        targetToSteal = t;
                        isQueuedTarget = true;
                        break; // 1つだけ奪う
                    }
                }
            }

            // B. 手動ロックが無い場合、現在ホバー中のシールドをチェック
            if (!targetToSteal) {
                auto hover = targetingComp_->GetHoverTarget();
                if (hover) {
                    if (auto debrisComp = hover->GetComponent<DebrisComponent>()) {
                        if (debrisComp->GetState() == DebrisState::BossOrbiting) {
                            targetToSteal = hover;
                        }
                    }
                }
            }
        }

        // 2. ターゲットが見つかった場合、ボスのシールドを奪う実行処理
        if (targetToSteal) {
            auto debrisComp = targetToSteal->GetComponent<DebrisComponent>();
            if (auto bossTarget = debrisComp->GetTarget().lock()) {
                if (auto bossComp = bossTarget->GetComponent<BossComponent>()) {
                    bossComp->RemoveShield(targetToSteal);
                }
            }
            
            debrisComp->SetState(DebrisState::Pulled);
            debrisComp->SetTarget(gameObject_->shared_from_this());
            debrisComp->SetOrbitParams(Random::GeneratorFloat(0.0f, orbitAngleRandomMax_), Random::GeneratorFloat(orbitRadiusMin_, orbitRadiusMax_));
            orbitingDebris_.push_back(targetToSteal);
            
            // キューにあったものを奪った場合は、手動ロックを解除する
            if (isQueuedTarget) {
                targetingComp_->ClearTargets();
            }
            return; // ボスから奪った場合はフリーガレキは吸わない
        }

        if (!debrisManager_) return;

        auto debrisObj = debrisManager_->ExtractNearestIdleDebris(transform->GetWorldPosition(), pullRadius_);
        if (debrisObj) {
            if (auto debrisComp = debrisObj->GetComponent<DebrisComponent>()) {
                debrisComp->SetState(DebrisState::Pulled);
                debrisComp->SetTarget(gameObject_->shared_from_this());
                debrisComp->SetOrbitParams(Random::GeneratorFloat(0.0f, orbitAngleRandomMax_), Random::GeneratorFloat(orbitRadiusMin_, orbitRadiusMax_));
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
    if (input->IsMouseButtonPressed(Mouse::Button::Right)) {
        size_t maxLockOn = orbitingDebris_.size();
        if (maxLockOn == 0) maxLockOn = 1; // シールド奪取用に最低1つはロック許可
        targetingComp_->MarkTarget(maxLockOn);
    }
}

void GravityPlayerComponent::HandleThrowInput() {
    auto input = BaseModel::GetIrufemiEngine()->GetInputManager();
    if (!input) return;

    // 左クリックで射撃
    if (input->IsMouseButtonPressed(Mouse::Button::Left) || input->IsKeyPressed('Q')) {
        if (orbitingDebris_.empty()) return;
        
        size_t lockonCount = targetingComp_ ? targetingComp_->GetQueuedTargets().size() : 0;
        
        if (lockonCount > 0) {
            // ロックオンしている場合は、ターゲットの数だけ発射する
            throwRemainingCount_ = static_cast<int>((std::min)(orbitingDebris_.size(), lockonCount));
        } else {
            // ノーロック時は1発だけ撃つ
            throwRemainingCount_ = 1;
        }

        isThrowing_ = true;
        throwTimer_ = throwInterval_; // 即座に1発目を撃つため
    }
}

void GravityPlayerComponent::UpdateThrowing() {
    if (throwRemainingCount_ <= 0 || orbitingDebris_.empty()) {
        isThrowing_ = false;
        if (targetingComp_) targetingComp_->ClearTargets(); // 弾切れ・または予定数撃ち切りでマークをクリア
        return;
    }

    auto engine = BaseModel::GetIrufemiEngine();
    throwTimer_ += engine->GetDeltaTime();

    if (throwTimer_ >= throwInterval_) {
        throwTimer_ = 0.0f;
        throwRemainingCount_--;

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
                if (throwTarget && throwTarget->GetIsActive()) {
                    comp->SetTarget(throwTarget);
                } else {
                    comp->SetTarget(std::weak_ptr<GameObject>());
                    Vector3 debrisPos = debris->GetComponent<TransformComponent>()->GetWorldPosition();

                    if (throwTarget) {
                        // ターゲットはいたが死んでいた場合、その死んだ座標へ直進させる
                        Vector3 deadPos = throwTarget->GetComponent<TransformComponent>()->GetWorldPosition();
                        Vector3 throwDir = Math::Normalize(Math::Subtract(deadPos, debrisPos));
                        comp->SetThrowDirection(throwDir);
                    } else {
                        // 完全なノーロック時の場合、マウスカーソルの奥へレイキャスト
                        auto cameraManager = engine->GetCameraManager();
                        auto inputManager = engine->GetInputManager();
                        if (cameraManager && cameraManager->GetActiveCamera() && inputManager) {
                            auto camera = cameraManager->GetActiveCamera();
                            float width = camera->GetViewportWidth();
                            float height = camera->GetViewportHeight();
                            Vector2 mousePos = inputManager->GetMousePosition();
                            
                            Matrix4x4 viewProjInv = Math::Inverse(camera->GetViewProjectionMatrix3D());
                            Ray ray = Math::ScreenPointToRay(mousePos, width, height, viewProjInv);
                            
                            RaycastHit hitInfo;
                            Vector3 targetPoint;
                            if (engine->GetCollisionManager()->Raycast(ray, hitInfo, noLockThrowDistance_)) {
                                targetPoint = hitInfo.hitPoint;
                            } else {
                                targetPoint = Math::Add(ray.origin, Math::Multiply(noLockThrowDistance_, ray.diff));
                            }
                            
                            Vector3 throwDir = Math::Normalize(Math::Subtract(targetPoint, debrisPos));
                            comp->SetThrowDirection(throwDir);
                        }
                    }
                }
            }
        }
    }
}
