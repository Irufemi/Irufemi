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
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/SkinnedMeshRendererComponent.h"
#include <algorithm>
#include <cmath>
#include "Engine/Core/Utility/Log.h"
#include <iostream>
#include "Framework/Component/Effect/ScreenEffectComponent.h"

void GravityPlayerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Max Orbit Count", &maxOrbitCount_);
    RegisterProperty("Pull Radius", &pullRadius_);
    RegisterProperty("No Lock Throw Dist", &noLockThrowDistance_);
    RegisterProperty("HP", &hp_);
    RegisterProperty("Max HP", &maxHp_);
    RegisterProperty("God Mode", &isGodMode_);
    RegisterProperty("Throw Interval", &throwInterval_);
    RegisterProperty("Orbit Radius Min", &orbitRadiusMin_);
    RegisterProperty("Orbit Radius Max", &orbitRadiusMax_);
    RegisterProperty("Orbit Angle Max", &orbitAngleRandomMax_);
}

void GravityPlayerComponent::Initialize() {
    orbitingDebris_.clear();
    isThrowing_ = false;
    throwTimer_ = 0.0f;
    invincibilityTimer_ = 0.0f;
    flashTimer_ = 0.0f;
    flashInterval_ = 0.1f;
    colorCached_ = false;
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
#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
    if (BaseModel::GetIrufemiEngine()->GetInputManager()->IsKeyPressed(VK_F9)) {
        isGodMode_ = !isGodMode_;
        Log::OutPutLog(std::cout, std::string("[GravityPlayer] God Mode ") + (isGodMode_ ? "ON\n" : "OFF\n"));
    }
#endif

    if (isDead_) return;

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

void GravityPlayerComponent::TakeDamage(int damage) {
    if (isDead_) return;
    
    if (isGodMode_) {
        Log::OutPutLog(std::cout, "[GravityPlayer] TakeDamage ignored (God Mode)\n");
        return;
    }

    if (IsInvincible()) {
        Log::OutPutLog(std::cout, "[GravityPlayer] TakeDamage ignored (Invincible)\n");
        return;
    }

    hp_ -= damage;
    Log::OutPutLog(std::cout, "[GravityPlayer] Took Damage! HP: " + std::to_string(hp_) + "\n");
    if (hp_ <= 0) {
        hp_ = 0;
        isDead_ = true;
        Log::OutPutLog(std::cout, "[GravityPlayer] Player Died!\n");
        
        // 自機が死んだときに自機のモデルの描画を切る
        if (auto mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
            mesh->SetVisible(false);
        } else if (auto skinned = gameObject_->GetComponent<SkinnedMeshRendererComponent>()) {
            skinned->SetVisible(false);
        }
        
        return;
    }

    Log::OutPutLog(std::cout, "[GravityPlayer] Triggering flashing...\n");
    invincibilityTimer_ = maxInvincibilityTime_;
    isFlashing_ = true;
    flashTimer_ = 0.0f;
    
    // TODO: 必要に応じてカメラシェイク発火を追加

    // ポストエフェクト演出の再生
    auto& comps = gameObject_->GetComponents();
    for (auto& comp : comps) {
        if (auto screenEffect = std::dynamic_pointer_cast<ScreenEffectComponent>(comp)) {
            screenEffect->Play();
        }
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

        auto transform = GetTransform();
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
            debrisComp->SetOrbitParams(Irufemi::Random::GeneratorFloat(0.0f, orbitAngleRandomMax_), Irufemi::Random::GeneratorFloat(orbitRadiusMin_, orbitRadiusMax_));
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
                debrisComp->SetOrbitParams(Irufemi::Random::GeneratorFloat(0.0f, orbitAngleRandomMax_), Irufemi::Random::GeneratorFloat(orbitRadiusMin_, orbitRadiusMax_));
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
                    Irufemi::Vector3 debrisPos = debris->GetComponent<TransformComponent>()->GetWorldPosition();

                    if (throwTarget) {
                        // ターゲットはいたが死んでいた場合、その死んだ座標へ直進させる
                        Irufemi::Vector3 deadPos = throwTarget->GetComponent<TransformComponent>()->GetWorldPosition();
                        Irufemi::Vector3 throwDir = Irufemi::Math::Normalize(Irufemi::Math::Subtract(deadPos, debrisPos));
                        comp->SetThrowDirection(throwDir);
                    } else {
                        // 完全なノーロック時の場合、マウスカーソルの奥へレイキャスト
                        auto cameraManager = engine->GetCameraManager();
                        auto inputManager = engine->GetInputManager();
                        if (cameraManager && cameraManager->GetActiveCamera() && inputManager) {
                            auto camera = cameraManager->GetActiveCamera();
                            float width = camera->GetViewportWidth();
                            float height = camera->GetViewportHeight();
                            Irufemi::Vector2 mousePos = inputManager->GetMousePosition();
                            
                            Irufemi::Matrix4x4 viewProjInv = Irufemi::Math::Inverse(camera->GetViewProjectionMatrix3D());
                            Irufemi::Ray ray = Irufemi::Math::ScreenPointToRay(mousePos, width, height, viewProjInv);
                            
                            RaycastHit hitInfo;
                            Irufemi::Vector3 targetPoint;
                            if (engine->GetCollisionManager()->Raycast(ray, hitInfo, noLockThrowDistance_)) {
                                targetPoint = hitInfo.hitPoint;
                            } else {
                                targetPoint = Irufemi::Math::Add(ray.origin, Irufemi::Math::Multiply(noLockThrowDistance_, ray.diff));
                            }
                            
                            Irufemi::Vector3 throwDir = Irufemi::Math::Normalize(Irufemi::Math::Subtract(targetPoint, debrisPos));
                            comp->SetThrowDirection(throwDir);
                        }
                    }
                }
            }
        }
    }
}
