#include "Environment/DebrisManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/VirtualEntity/VirtualEntityManagerComponent.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include "Framework/Scene/SceneSerializer.h"
#include "Framework/Scene/BaseScene.h"
#include "Environment/DebrisComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Core/Math/Random/Random.h"
#include "Renderer/Camera/CameraManager.h"
#include "Renderer/Camera/Camera.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h"
#include "Core/Type/PrimitiveType.h"
#include "Renderer/Pipeline/PSOManager.h"
#include "Player/TargetableComponent.h"
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Core/Utility/Log.h"
#include <iostream>
#include <algorithm>
#include "Effects/EffectManagerComponent.h"
#include "Renderer/System/VoxelParticle/VoxelParticleManager.h"

void DebrisManagerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Debris Pull Speed", &debrisPullSpeed_);
    RegisterProperty("Debris Throw Speed", &debrisThrowSpeed_);
    RegisterProperty("Debris Orbit Speed", &debrisOrbitSpeed_);
    RegisterProperty("Debris Orbit Radius", &debrisOrbitRadius_);
    RegisterProperty("Debris Damage (Boss)", &debrisDamage_);
    RegisterProperty("Debris Damage (Enemy)", &debrisEnemyDamage_);
    RegisterProperty("Debris Pull Y Offset", &debrisPullYOffset_);
    RegisterProperty("Camera Shake Intensity", &cameraShakeIntensity_);
    RegisterProperty("Camera Shake Duration(Frames)", &cameraShakeDurationFrames_);
    RegisterProperty("Player Aura Color", &playerAuraColor_);
    RegisterProperty("Boss Aura Color", &bossAuraColor_);
    RegisterProperty("Idle Aura Color", &idleAuraColor_);
    RegisterProperty("Catch Distance Sq", &catchDistanceSq_);
    RegisterProperty("Boss Shield Radius", &bossShieldRadius_);

    RegisterProperty("Debris Base Scale", &debrisBaseScale_);
    RegisterProperty("Debris Collider Radius", &colliderRadius_);
    RegisterProperty("Debris Aura Scale", &auraScale_);
    RegisterProperty("Max Throw Distance", &maxThrowDistance_);
}

void DebrisManagerComponent::Initialize() {
    std::string configPath = "resources/GameData/DebrisPalette.json";
    std::ifstream file(configPath);
    if (!file.is_open()) {
        Log::OutPutLog(std::cout, "Failed to load DebrisPalette.json\n");
        return;
    }

    nlohmann::json j;
    file >> j;

    const auto& variationsJson = j["variations"];
    int varIndex = 0;
    for (const auto& v : variationsJson) {
        DebrisVariation var;
        var.id = v["id"].get<std::string>();
        var.modelPath = v["modelPath"].get<std::string>();
        var.maxVirtualCount = v["maxVirtualCount"].get<int>();
        var.maxPoolSize = v["maxPoolSize"].get<int>();
        var.spawnWeight = v["spawnWeight"].get<int>();

        var.poolObject = std::make_shared<GameObject>("DebrisPool_" + var.id);

        auto batchRenderer = var.poolObject->AddComponent<ModelBatchRendererComponent>();
        batchRenderer->LoadModel(var.modelPath);

        var.virtualManager = var.poolObject->AddComponent<VirtualEntityManagerComponent>().get();

        auto debrisFactory = [this, varIndex]() -> std::shared_ptr<GameObject> {
            auto obj = std::make_shared<GameObject>("Debris");
            auto transform = obj->GetTransform();
            transform->SetScale(debrisBaseScale_);

            auto debrisComp = obj->AddComponent<DebrisComponent>();
            debrisComp->SetVariationIndex(varIndex);

            obj->AddComponent<TargetableComponent>();

            auto collider = obj->AddComponent<SphereColliderComponent>();
            collider->isTrigger_ = true;
            collider->SetLocalRadius(colliderRadius_);

            // --- Aura (EnergyCore) ---
            auto aura = std::make_shared<GameObject>("DebrisAura");
            auto auraTransform = aura->GetTransform();
            auraTransform->SetScale(auraScale_);

            auto auraModel = aura->AddComponent<PrimitiveRendererComponent>();
            auraModel->Initialize();
            auraModel->SetShape(Irufemi::PrimitiveType::Sphere);

            if (auto primitive = static_cast<Primitive3DObject*>(auraModel->GetRenderable())) {
                auto pso = BaseModel::GetIrufemiEngine()->GetPSOManager()->GetPSO(
                    "EnergyCore", Irufemi::BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable,
                    PSOManager::CullMode::Back);
                primitive->SetCustomPSO(pso);
                primitive->SetIsTransparent(true);
                primitive->SetColor(idleAuraColor_);
            }

            aura->SetIsActive(false);
            obj->AddChild(aura);
            // -------------------------

            obj->SetIsActive(false);
            return obj;
        };

        var.virtualManager->Setup(var.maxPoolSize, var.maxVirtualCount + 500, debrisFactory);
        var.animDataList.resize(var.maxVirtualCount + 500);

        if (gameObject_) {
            gameObject_->AddChild(var.poolObject);
        }

        variations_.push_back(var);
        varIndex++;
    }
}

void DebrisManagerComponent::Update() {
    auto input = BaseModel::GetIrufemiEngine()->GetInputManager();

    auto spawnDebris = [&](int count) {
        Irufemi::Vector3 spawnBase = {0.0f, 0.0f, 0.0f};
        Irufemi::Vector3 forward = {0.0f, 0.0f, 1.0f};
        Irufemi::Vector3 right = {1.0f, 0.0f, 0.0f};

        auto scene = gameObject_->GetScene();
        if (scene) {
            auto playerObj = scene->FindGameObject("Player");
            if (playerObj) {
                if (auto t = playerObj->GetComponent<TransformComponent>()) {
                    spawnBase = t->GetPosition();
                    float yaw = t->GetRotation().y;
                    forward = {std::sin(yaw), 0.0f, std::cos(yaw)};
                    right = {std::cos(yaw), 0.0f, -std::sin(yaw)};
                }
            }
        }

        int totalWeight = 0;
        for (const auto& var : variations_) {
            totalWeight += var.spawnWeight;
        }

        if (totalWeight <= 0) {
            return;
        }

        for (int i = 0; i < count; ++i) {
            float distFwd = (count > 100) ? Irufemi::Random::GeneratorFloat(10.0f, 300.0f)
                                          : Irufemi::Random::GeneratorFloat(30.0f, 80.0f);
            float distRight = (count > 100) ? Irufemi::Random::GeneratorFloat(-150.0f, 150.0f)
                                            : Irufemi::Random::GeneratorFloat(-20.0f, 20.0f);
            float height = (count > 100) ? Irufemi::Random::GeneratorFloat(-10.0f, 100.0f)
                                         : Irufemi::Random::GeneratorFloat(-5.0f, 15.0f);

            Irufemi::Vector3 pos = {spawnBase.x + forward.x * distFwd + right.x * distRight, spawnBase.y + height,
                                    spawnBase.z + forward.z * distFwd + right.z * distRight};

            int randW = static_cast<int>(Irufemi::Random::GeneratorUint64(0, totalWeight - 1));
            int selectedIndex = 0;
            int currentW = 0;
            for (size_t v = 0; v < variations_.size(); ++v) {
                currentW += variations_[v].spawnWeight;
                if (randW < currentW) {
                    selectedIndex = static_cast<int>(v);
                    break;
                }
            }

            auto& var = variations_[selectedIndex];
            int vid = var.virtualManager->AddVirtualInstance(pos, {0, 0, 0}, {0.5f, 0.5f, 0.5f});
            if (vid >= 0) {
                DebrisAnimData anim;
                anim.baseIdleY_ = pos.y;
                anim.idleTimeY_ = Irufemi::Random::GeneratorFloat(0.0f, 100.0f);
                var.animDataList[vid] = anim;
                var.activeIds.push(vid);
            }
        }

        for (auto& var : variations_) {
            while (var.activeIds.size() > static_cast<size_t>(var.maxVirtualCount)) {
                int oldestId = var.activeIds.front();
                var.activeIds.pop();
                var.virtualManager->RemoveVirtualInstance(oldestId);
            }
        }
    };

    if (input->IsKeyPressed('1') || input->IsKeyPressedDIK(0x02)) {
        spawnDebris(10);
    }
    if (input->IsKeyPressed('9') || input->IsKeyPressedDIK(0x0A)) {
        spawnDebris(10000);
    }

    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (deltaTime > 0.0f) {
        UpdatePulledDebris(deltaTime);
        UpdateOrbitingDebris(deltaTime);
        UpdateBossOrbitingDebris(deltaTime);
        UpdateThrownDebris(deltaTime);
    }

    for (auto& debris : pendingReleases_) {
        ReleaseDebris(debris);
    }
    pendingReleases_.clear();

    for (auto& pair : pendingDestroys_) {
        NotifyDestroyed(pair.first, pair.second);
    }
    pendingDestroys_.clear();
}

std::shared_ptr<GameObject> DebrisManagerComponent::GetDebris() {
    if (variations_.empty()) {
        return nullptr;
    }

    // Boss用などは一旦0番（Archwayや固定のもの）を渡しておく
    auto& var = variations_[0];
    int id = var.virtualManager->AddVirtualInstance({0, 0, 0}, {0, 0, 0}, {0.5f, 0.5f, 0.5f});
    auto obj = var.virtualManager->Promote(id);
    if (obj) {
        auto comp = obj->GetComponent<DebrisComponent>();
        if (comp) {
            comp->SetManager(this);
            comp->SetVirtualId(-1);
            comp->SetVariationIndex(0);
        }
    }
    return obj;
}

void DebrisManagerComponent::ReleaseDebris(std::shared_ptr<GameObject> debris) {
    if (!debris) {
        return;
    }
    auto comp = debris->GetComponent<DebrisComponent>();
    if (comp) {
        int vid = comp->GetVirtualId();
        int vIndex = comp->GetVariationIndex();
        if (vid >= 0 && vIndex >= 0 && vIndex < variations_.size()) {
            variations_[vIndex].virtualManager->Demote(vid);
        } else if (vIndex >= 0 && vIndex < variations_.size()) {
            variations_[vIndex].virtualManager->ReleaseGameObject(debris);
        } else {
            debris->SetIsActive(false);
        }
    }
}

void DebrisManagerComponent::MarkForRelease(std::shared_ptr<GameObject> debris) {
    if (debris) {
        pendingReleases_.push_back(debris);
    }
}

std::shared_ptr<GameObject> DebrisManagerComponent::ExtractNearestIdleDebris(const Irufemi::Vector3& pos,
                                                                             float radius) {
    float bestDistSq = radius * radius;
    int bestId = -1;
    int bestVarIndex = -1;
    std::shared_ptr<GameObject> bestPromotedObj = nullptr;

    for (size_t v = 0; v < variations_.size(); ++v) {
        auto& virtualInstances = variations_[v].virtualManager->GetDenseInstances();
        for (const auto& vi : virtualInstances) {
            float dx, dy, dz;
            bool isValid = false;

            if (!vi.isPromoted_) {
                dx = vi.position_.x - pos.x;
                dy = vi.position_.y - pos.y;
                dz = vi.position_.z - pos.z;
                isValid = true;
            } else {
                auto obj = variations_[v].virtualManager->Promote(vi.id_);
                if (obj && obj->GetIsActive()) {
                    if (auto comp = obj->GetComponent<DebrisComponent>()) {
                        if (comp->GetState() == DebrisState::Idle) {
                            if (auto t = obj->GetTransform()) {
                                dx = t->GetPosition().x - pos.x;
                                dy = t->GetPosition().y - pos.y;
                                dz = t->GetPosition().z - pos.z;
                                isValid = true;
                            }
                        }
                    }
                }
            }

            if (isValid) {
                float distSq = dx * dx + dy * dy + dz * dz;
                if (distSq <= bestDistSq) {
                    bestDistSq = distSq;
                    bestId = vi.id_;
                    bestVarIndex = static_cast<int>(v);

                    if (vi.isPromoted_) {
                        bestPromotedObj = variations_[v].virtualManager->Promote(vi.id_);
                    } else {
                        bestPromotedObj = nullptr;
                    }
                }
            }
        }
    }

    if (bestId >= 0 && bestVarIndex >= 0) {
        auto obj = bestPromotedObj ? bestPromotedObj : variations_[bestVarIndex].virtualManager->Promote(bestId);
        if (obj) {
            auto comp = obj->GetComponent<DebrisComponent>();
            if (comp) {
                comp->SetVirtualId(bestId);
                comp->SetVariationIndex(bestVarIndex);
                comp->SetManager(this);
                comp->SetState(DebrisState::Idle);
            }
            return obj;
        }
    }
    return nullptr;
}

void DebrisManagerComponent::NotifyDestroyed(int virtualId, int variationIndex) {
    if (variationIndex >= 0 && variationIndex < variations_.size()) {
        variations_[variationIndex].virtualManager->RemoveVirtualInstance(virtualId);
    }
}

void DebrisManagerComponent::MarkForDestroy(int virtualId, int variationIndex) {
    pendingDestroys_.push_back({virtualId, variationIndex});
}

void DebrisManagerComponent::RegisterDebris(DebrisComponent* debris, DebrisState state) {
    switch (state) {
    case DebrisState::Pulled:
        pulledDebris_.push_back(debris);
        break;
    case DebrisState::Orbiting:
        orbitingDebris_.push_back(debris);
        break;
    case DebrisState::BossOrbiting:
        bossOrbitingDebris_.push_back(debris);
        break;
    case DebrisState::Thrown:
        thrownDebris_.push_back(debris);
        break;
    default:
        break;
    }
}

void DebrisManagerComponent::UnregisterDebris(DebrisComponent* debris, DebrisState state) {
    auto remove_func = [debris](std::vector<DebrisComponent*>& vec) {
        auto it = std::find(vec.begin(), vec.end(), debris);
        if (it != vec.end()) {
            vec.erase(it);
        }
    };
    switch (state) {
    case DebrisState::Pulled:
        remove_func(pulledDebris_);
        break;
    case DebrisState::Orbiting:
        remove_func(orbitingDebris_);
        break;
    case DebrisState::BossOrbiting:
        remove_func(bossOrbitingDebris_);
        break;
    case DebrisState::Thrown:
        remove_func(thrownDebris_);
        break;
    default:
        break;
    }
}

void DebrisManagerComponent::UpdatePulledDebris(float deltaTime) {
    float catchDistSq = GetCatchDistanceSq();
    float pullSpeed = GetDebrisPullSpeed();
    float pullYOffset = GetDebrisPullYOffset();

    for (int i = (int)pulledDebris_.size() - 1; i >= 0; --i) {
        DebrisComponent* debris = pulledDebris_[i];
        if (!debris || !debris->gameObject_) {
            continue;
        }
        auto transform = debris->GetTransform();
        if (!transform) {
            continue;
        }

        if (auto target = debris->targetObject_.lock()) {
            auto targetTransform = target->GetComponent<TransformComponent>();
            if (targetTransform) {
                Irufemi::Vector3 targetPos = targetTransform->GetWorldPosition();
                Irufemi::Vector3 pos = transform->GetWorldPosition();
                Irufemi::Vector3 diff = {targetPos.x - pos.x, targetPos.y + pullYOffset - pos.y, targetPos.z - pos.z};
                pos.x += diff.x * pullSpeed * deltaTime;
                pos.y += diff.y * pullSpeed * deltaTime;
                pos.z += diff.z * pullSpeed * deltaTime;
                transform->SetPosition(pos);

                float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                if (distSq < catchDistSq) {
                    debris->SetState(DebrisState::Orbiting);
                }
            }
        }
    }
}

void DebrisManagerComponent::UpdateOrbitingDebris(float deltaTime) {
    float orbitSpeed = GetDebrisOrbitSpeed();

    for (int i = (int)orbitingDebris_.size() - 1; i >= 0; --i) {
        DebrisComponent* debris = orbitingDebris_[i];
        if (!debris || !debris->gameObject_) {
            continue;
        }
        auto transform = debris->GetTransform();
        if (!transform) {
            continue;
        }

        if (auto target = debris->targetObject_.lock()) {
            auto targetTransform = target->GetComponent<TransformComponent>();
            if (targetTransform) {
                debris->orbitAngle_ += orbitSpeed * deltaTime;

                Irufemi::Vector3 offset = {std::cos(debris->orbitAngle_) * debris->orbitRadius_,
                                           std::sin(debris->orbitAngle_ * 2.0f) * 0.5f + 1.0f,
                                           std::sin(debris->orbitAngle_) * debris->orbitRadius_};

                Irufemi::Vector3 pos = transform->GetWorldPosition();
                pos.x = targetTransform->GetWorldPosition().x + offset.x;
                pos.y = targetTransform->GetWorldPosition().y + offset.y;
                pos.z = targetTransform->GetWorldPosition().z + offset.z;
                transform->SetPosition(pos);
            }
        }
    }
}

void DebrisManagerComponent::UpdateBossOrbitingDebris(float deltaTime) {
    float shieldRotationSpeed = 1.0f;
    float currentRadiusBase = GetBossShieldRadius();

    for (int i = (int)bossOrbitingDebris_.size() - 1; i >= 0; --i) {
        DebrisComponent* debris = bossOrbitingDebris_[i];
        if (!debris || !debris->gameObject_) {
            continue;
        }
        auto transform = debris->GetTransform();
        if (!transform) {
            continue;
        }

        if (auto target = debris->targetObject_.lock()) {
            auto targetTransform = target->GetComponent<TransformComponent>();
            if (targetTransform) {
                debris->bossOrbitAngleX_ += debris->bossOrbitSpeedX_ * shieldRotationSpeed * deltaTime;
                debris->bossOrbitAngleY_ += debris->bossOrbitSpeedY_ * shieldRotationSpeed * deltaTime;
                debris->bossOrbitAngleZ_ += debris->bossOrbitSpeedZ_ * shieldRotationSpeed * deltaTime;

                Irufemi::Matrix4x4 rotMatrix = Irufemi::Math::MakeRotateXYZMatrix(
                    Irufemi::Vector3{debris->bossOrbitAngleX_, debris->bossOrbitAngleY_, debris->bossOrbitAngleZ_});
                float currentRadius = currentRadiusBase + debris->bossOrbitRadiusOffset_;
                Irufemi::Vector3 baseOffset = {0, 0, currentRadius};
                Irufemi::Vector3 localPos = Irufemi::Math::TransformNormal(baseOffset, rotMatrix);

                Irufemi::Vector3 pos = transform->GetWorldPosition();
                pos.x = targetTransform->GetWorldPosition().x + localPos.x;
                pos.y = targetTransform->GetWorldPosition().y + localPos.y;
                pos.z = targetTransform->GetWorldPosition().z + localPos.z;
                transform->SetPosition(pos);

                transform->SetRotation({debris->bossOrbitAngleX_ * 2.0f, debris->bossOrbitAngleY_ * 2.0f,
                                        debris->bossOrbitAngleZ_ * 2.0f});
            }
        }
    }
}

void DebrisManagerComponent::UpdateThrownDebris(float deltaTime) {
    float throwSpeed = GetDebrisThrowSpeed();
    float maxDistSq = GetMaxThrowDistanceSq();
    EffectManagerComponent* effectManager = nullptr;
    auto voxelManager = BaseModel::GetIrufemiEngine()->GetVoxelParticleManager();

    if (gameObject_ && gameObject_->GetScene()) {
        if (auto go = gameObject_->GetScene()->FindGameObject("EffectManager")) {
            effectManager = go->GetComponent<EffectManagerComponent>();
        }
    }

    for (int i = (int)thrownDebris_.size() - 1; i >= 0; --i) {
        DebrisComponent* debris = thrownDebris_[i];
        if (!debris || !debris->gameObject_) {
            continue;
        }
        auto transform = debris->GetTransform();
        if (!transform) {
            continue;
        }

        bool updatedWithTarget = false;
        if (auto target = debris->targetObject_.lock()) {
            if (target->GetIsActive()) {
                auto targetTransform = target->GetComponent<TransformComponent>();
                if (targetTransform) {
                    Irufemi::Vector3 targetPos = targetTransform->GetWorldPosition();
                    Irufemi::Vector3 pos = transform->GetWorldPosition();
                    Irufemi::Vector3 diff = {targetPos.x - pos.x, targetPos.y - pos.y, targetPos.z - pos.z};
                    float len = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
                    float moveDist = throwSpeed * deltaTime;
                    if (len > 0.001f) {
                        if (len <= moveDist) {
                            debris->throwDirection_ = {diff.x / len, diff.y / len, diff.z / len};
                            pos.x += diff.x;
                            pos.y += diff.y;
                            pos.z += diff.z;
                            transform->SetPosition(pos);
                            continue; // 衝突処理はOnCollisionEnterに任せるためここでは移動のみ
                        } else {
                            debris->throwDirection_ = {diff.x / len, diff.y / len, diff.z / len};
                        }
                    }
                    updatedWithTarget = true;
                }
            }
        }

        Irufemi::Vector3 pos = transform->GetWorldPosition();
        pos.x += debris->throwDirection_.x * throwSpeed * deltaTime;
        pos.y += debris->throwDirection_.y * throwSpeed * deltaTime;
        pos.z += debris->throwDirection_.z * throwSpeed * deltaTime;
        transform->SetPosition(pos);

        float dx = pos.x - debris->throwOrigin_.x;
        float dy = pos.y - debris->throwOrigin_.y;
        float dz = pos.z - debris->throwOrigin_.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq > maxDistSq) {
            if (effectManager) {
                effectManager->PlayEffect(debris->hitEffectKey_, pos);
            }
            if (voxelManager) {
                VoxelEmitter p{};
                p.particleType = 5; // DebrisExplosive
                p.lifeTime = 1.0f;
                p.gravity = 5.0f;
                p.dispersion = 12.0f;
                p.scale = {0.5f, 0.5f, 0.5f};

                Irufemi::Vector4 aura =
                    (debris->state_ == DebrisState::BossOrbiting) ? GetBossAuraColor() : GetPlayerAuraColor();
                Irufemi::Vector4 rockColor = {1.5f, 1.2f, 1.0f, 1.0f};
                p.startColor = {rockColor.x + aura.x * 2.0f, rockColor.y + aura.y * 2.0f, rockColor.z + aura.z * 2.0f,
                                1.0f};
                p.endColor = {0.2f, 0.2f, 0.2f, 1.0f};
                p.dissolveEdgeColor = aura;

                voxelManager->PlayExplosion(debris->explosionModelPath_, pos, {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, p,
                                            {2, 2, 2});
            }
            MarkForRelease(debris->gameObject_->shared_from_this());
        }
    }
}
