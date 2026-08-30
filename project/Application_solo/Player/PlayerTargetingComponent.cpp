#include "Player/PlayerTargetingComponent.h"
#include "Player/TargetableComponent.h"
#include "RailMechanics/RailShooterEnemyComponent.h"
#include "Combat/Boss/BossComponent.h"
#include "Environment/DebrisComponent.h"
#include "UI/LockonMarkerUIComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Platform/Input/InputManager.h"
#include "Renderer/Camera/CameraManager.h"
#include "Renderer/Camera/Camera.h"
#include "Physics/CollisionManager.h"
#include "Core/Math/MathFunction.h"
#include "Core/Shape/LinePrimitive.h"
#include "Renderer/System/Core/BaseModel.h"
#include <algorithm>
#include <limits>
#include <cmath>

void PlayerTargetingComponent::Initialize() {
    // UIコンポーネントを検索
    auto scene = gameObject_->GetScene();
    if (scene) {
        for (auto obj : scene->GetGameObjects()) {
            if (auto ui = obj->GetComponent<LockonMarkerUIComponent>()) {
                lockonMarkerUI_ = ui;
                break;
            }
        }
    }
}

void PlayerTargetingComponent::Start() {}

void PlayerTargetingComponent::Update() {
    // 死んだオブジェクトなどをキューから削除する
    queuedTargets_.erase(std::remove_if(queuedTargets_.begin(), queuedTargets_.end(),
                                        [](const std::shared_ptr<GameObject>& obj) {
                                            if (!obj || !obj->GetIsActive())
                                                return true;

                                            // 生死判定
                                            if (auto enemyComp = obj->GetComponent<RailShooterEnemyComponent>()) {
                                                if (!enemyComp->IsAlive())
                                                    return true;
                                            } else if (auto bossComp = obj->GetComponent<BossComponent>()) {
                                                if (!bossComp->IsCoreExposed())
                                                    return true;
                                            } else if (auto debrisComp = obj->GetComponent<DebrisComponent>()) {
                                                if (debrisComp->GetState() != DebrisState::BossOrbiting)
                                                    return true;
                                            }

                                            return false;
                                        }),
                         queuedTargets_.end());

    UpdateHoverTarget();

    if (!lockonMarkerUI_) {
        auto scene = gameObject_->GetScene();
        if (scene) {
            for (auto obj : scene->GetGameObjects()) {
                if (auto ui = obj->GetComponent<LockonMarkerUIComponent>()) {
                    lockonMarkerUI_ = ui;
                    break;
                }
            }
        }
    }

    if (lockonMarkerUI_) {
        lockonMarkerUI_->SetMaxLockonCount(maxLockonCount_);
        std::vector<std::shared_ptr<GameObject>> displayTargets = queuedTargets_;
        if (hoverTarget_) {
            displayTargets.push_back(hoverTarget_);
        }
        lockonMarkerUI_->SyncTargets(displayTargets);
    }
}

void PlayerTargetingComponent::OnRegisterProperties() {}

void PlayerTargetingComponent::UpdateHoverTarget() {
    hoverTarget_ = nullptr;

    auto engine = BaseModel::GetIrufemiEngine();
    auto cameraManager = engine->GetCameraManager();
    if (!cameraManager || !cameraManager->GetActiveCamera())
        return;
    auto camera = cameraManager->GetActiveCamera();

    Irufemi::Matrix4x4 viewProj = camera->GetViewProjectionMatrix3D();
    float viewWidth = camera->GetViewportWidth();
    float viewHeight = camera->GetViewportHeight();

    auto inputManager = engine->GetInputManager();
    Irufemi::Vector2 screenCenter =
        inputManager ? inputManager->GetMousePosition() : Irufemi::Vector2{viewWidth * 0.5f, viewHeight * 0.5f};
    float currentTime = engine->GetTotalTime();

    // 1. 保留中の非同期レイキャストをポーリングして視線キャッシュを更新
    for (auto it = visibilityCache_.begin(); it != visibilityCache_.end();) {
        GameObject* objPtr = it->first;
        TargetVisibilityCache& cache = it->second;

        // オブジェクトが破棄されていたらキャッシュから削除
        if (!objPtr || !objPtr->GetIsActive()) {
            it = visibilityCache_.erase(it);
            continue;
        }

        if (cache.pendingTask) {
            if (cache.pendingTask->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                auto result = cache.pendingTask->get();
                bool hit = result.first;
                RaycastHit hitInfo = result.second;

                bool canSee = true;
                auto transform = objPtr->GetComponent<TransformComponent>();
                if (transform) {
                    Irufemi::Vector3 targetPos = transform->GetWorldPosition();
                    Irufemi::Vector3 cameraPos = camera->GetTranslate();
                    float dist3D = Irufemi::Math::Length(Irufemi::Math::Subtract(targetPos, cameraPos));

                    if (hit && hitInfo.hitObject != nullptr) {
                        if (hitInfo.hitObject != objPtr && hitInfo.distance < dist3D - 1.0f) {
                            canSee = false; // 障害物に遮蔽されている
                        }
                    }
                }
                cache.canSee = canSee;
                cache.pendingTask.reset();
            }
        }
        ++it;
    }

    std::shared_ptr<GameObject> bestTarget = nullptr;
    float bestScore = (std::numeric_limits<float>::max)();

    auto scene = gameObject_->GetScene();
    if (!scene)
        return;
    auto playerObj = gameObject_;

    // 2. ターゲット候補のスコアリングと評価
    for (auto targetComp : TargetableComponent::GetTargets()) {
        auto obj = targetComp->GetGameObject();
        if (!obj || !obj->GetIsActive())
            continue;

        bool isTargetable = false;
        if (auto enemyComp = obj->GetComponent<RailShooterEnemyComponent>()) {
            if (enemyComp->IsAlive())
                isTargetable = true;
        } else if (auto bossComp = obj->GetComponent<BossComponent>()) {
            if (bossComp->IsCoreExposed())
                isTargetable = true;
        } else if (auto debrisComp = obj->GetComponent<DebrisComponent>()) {
            if (debrisComp->GetState() == DebrisState::BossOrbiting)
                isTargetable = true;
        }

        if (isTargetable) {
            auto transform = obj->GetComponent<TransformComponent>();
            if (transform) {
                Irufemi::Vector3 worldPos = transform->GetWorldPosition();
                Irufemi::Vector3 clipPos = Irufemi::Math::Transform(worldPos, viewProj);

                if (clipPos.z >= 0.0f && clipPos.z <= 1.0f) {
                    float screenX = (clipPos.x + 1.0f) * 0.5f * viewWidth;
                    float screenY = (1.0f - clipPos.y) * 0.5f * viewHeight;

                    float dx = screenX - screenCenter.x;
                    float dy = screenY - screenCenter.y;
                    float dist2DSq = dx * dx + dy * dy;

                    if (dist2DSq <= lockonRadius2D_ * lockonRadius2D_) {
                        Irufemi::Vector3 cameraPos = camera->GetTranslate();
                        Irufemi::Vector3 toTarget = Irufemi::Math::Subtract(worldPos, cameraPos);
                        float dist3D = Irufemi::Math::Length(toTarget);

                        float score = std::sqrt(dist2DSq) * weight2D_ + dist3D * weight3D_;

                        if (score < bestScore) {
                            auto& cache = visibilityCache_[obj];

                            // 0.1秒以上経過していれば、非同期レイキャストを発行（Amortization）
                            if (currentTime - cache.lastCheckTime > 0.1f && !cache.pendingTask) {
                                cache.lastCheckTime = currentTime;
                                Irufemi::Vector3 dir = Irufemi::Math::Normalize(toTarget);
                                Irufemi::Ray ray;
                                ray.origin = cameraPos;
                                ray.diff = dir;

                                cache.pendingTask = std::make_shared<std::future<std::pair<bool, RaycastHit>>>(
                                    engine->GetCollisionManager()->RaycastAsync(engine->GetThreadPool(), ray,
                                                                                dist3D + 10.0f, 0xFFFFFFFF, playerObj));
                            }

                            // 非同期判定中の場合は、過去のキャッシュ(canSee)を利用して即座に評価を続ける
                            if (cache.canSee) {
                                bestScore = score;
                                bestTarget = obj->shared_from_this();
                            }
                        }
                    }
                }
            }
        }
    }

    hoverTarget_ = bestTarget;
}

void PlayerTargetingComponent::MarkTarget(size_t maxLockOn) {
    if (queuedTargets_.size() >= maxLockOn)
        return;

    if (hoverTarget_) {
        queuedTargets_.push_back(hoverTarget_);
    }
}

void PlayerTargetingComponent::ClearTargets() {
    queuedTargets_.clear();
}

std::shared_ptr<GameObject> PlayerTargetingComponent::PopTarget() {
    if (queuedTargets_.empty())
        return nullptr;
    auto target = queuedTargets_.front();
    queuedTargets_.erase(queuedTargets_.begin());
    return target;
}
