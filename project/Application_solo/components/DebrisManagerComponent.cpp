#include "DebrisManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/VirtualEntity/VirtualEntityManagerComponent.h"
#include "Framework/SceneSerializer.h"
#include "Framework/BaseScene.h"
#include "DebrisComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Engine/Core/Math/Random/Random.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h"
#include "Engine/Core/Type/PrimitiveType.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"
#include "TargetableComponent.h"
#include <cmath>


void DebrisManagerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Pool Size", &poolSize_);
    RegisterProperty("Max Virtual Instances", &maxVirtualInstances_);
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
}

void DebrisManagerComponent::Initialize() {
    virtualManager_ = gameObject_->GetComponent<VirtualEntityManagerComponent>();
    if (!virtualManager_) {
        virtualManager_ = gameObject_->AddComponent<VirtualEntityManagerComponent>().get();
    }

    auto debrisFactory = [this]() -> std::shared_ptr<GameObject> {
        auto obj = std::make_shared<GameObject>("Debris");
        auto transform = obj->AddComponent<TransformComponent>();
        transform->SetScale(debrisBaseScale_); 
        
        obj->AddComponent<DebrisComponent>();
        obj->AddComponent<TargetableComponent>();
        
        auto collider = obj->AddComponent<SphereColliderComponent>();
        collider->isTrigger_ = true;
        collider->SetLocalRadius(colliderRadius_);
        
        // --- Aura (EnergyCore) ---
        auto aura = std::make_shared<GameObject>("DebrisAura");
        auto auraTransform = aura->AddComponent<TransformComponent>();
        auraTransform->SetScale(auraScale_);
        
        auto auraModel = aura->AddComponent<PrimitiveRendererComponent>();
        auraModel->Initialize();
        auraModel->SetShape(PrimitiveType::Sphere); // 軽量なプリミティブ球を使用
        
        if (auto primitive = static_cast<Primitive3DObject*>(auraModel->GetRenderable())) {
            auto pso = BaseModel::GetIrufemiEngine()->GetPSOManager()->GetPSO("EnergyCore", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::Back);
            primitive->SetCustomPSO(pso);
            primitive->SetIsTransparent(true); // ★半透明パスでZソートして描画させる
            primitive->SetColor(idleAuraColor_);
        }
        
        aura->SetIsActive(false); // 通常時は非表示
        obj->AddChild(aura);
        // -------------------------

        obj->SetIsActive(false);

        // if (gameObject_) {
        //     gameObject_->AddChild(obj);
        // }
        return obj;
    };

    virtualManager_->Setup(poolSize_, maxVirtualInstances_, debrisFactory);

    animDataList_.resize(maxVirtualInstances_);
    while (!activeIds_.empty()) activeIds_.pop();
}

void DebrisManagerComponent::Update() {
    auto input = BaseModel::GetIrufemiEngine()->GetInputManager();
    // デバッグ用: 1キーを押したら10個ランダムな場所にスポーンさせる
    if (input->IsKeyPressed('1') || input->IsKeyPressedDIK(0x02 /*DIK_1*/)) {
        Vector3 spawnBase = {0.0f, 0.0f, 0.0f};
        Vector3 forward = {0.0f, 0.0f, 1.0f};
        Vector3 right = {1.0f, 0.0f, 0.0f};
        
        auto scene = gameObject_->GetScene();
        if (scene) {
            auto playerObj = scene->FindGameObject("Player");
            if (playerObj) {
                if (auto t = playerObj->GetComponent<TransformComponent>()) {
                    spawnBase = t->GetPosition();
                    float yaw = t->GetRotation().y;
                    forward = { std::sin(yaw), 0.0f, std::cos(yaw) };
                    right = { std::cos(yaw), 0.0f, -std::sin(yaw) };
                }
            }
        }

        for (int i = 0; i < 10; ++i) {
            float distFwd = Random::GeneratorFloat(30.0f, 80.0f);
            float distRight = Random::GeneratorFloat(-20.0f, 20.0f);
            float height = Random::GeneratorFloat(-5.0f, 15.0f);
            
            Vector3 pos = {
                spawnBase.x + forward.x * distFwd + right.x * distRight,
                spawnBase.y + height,
                spawnBase.z + forward.z * distFwd + right.z * distRight
            };
            
            int vid = virtualManager_->AddVirtualInstance(pos, {0,0,0}, {0.5f, 0.5f, 0.5f});
            if (vid >= 0) {
                DebrisAnimData anim;
                anim.baseIdleY_ = pos.y;
                anim.idleTimeY_ = Random::GeneratorFloat(0.0f, 100.0f);
                animDataList_[vid] = anim;
                activeIds_.push(vid);
            }
        }
        
        // プール上限の管理
        // 実体プール(poolSize_)ではなく、仮想インスタンスの上限で管理する
        while (activeIds_.size() > static_cast<size_t>(maxVirtualInstances_ - 100)) {
            int oldestId = activeIds_.front();
            activeIds_.pop();
            virtualManager_->RemoveVirtualInstance(oldestId);
        }
    }

    // デバッグ用: 9キーを押したら10,000個を一気にスポーンさせる（ストレステスト）
    if (input->IsKeyPressed('9') || input->IsKeyPressedDIK(0x0A /*DIK_9*/)) {
        Vector3 spawnBase = {0.0f, 0.0f, 0.0f};
        Vector3 forward = {0.0f, 0.0f, 1.0f};
        Vector3 right = {1.0f, 0.0f, 0.0f};
        
        auto scene = gameObject_->GetScene();
        if (scene) {
            auto playerObj = scene->FindGameObject("Player");
            if (playerObj) {
                if (auto t = playerObj->GetComponent<TransformComponent>()) {
                    spawnBase = t->GetPosition();
                    float yaw = t->GetRotation().y;
                    forward = { std::sin(yaw), 0.0f, std::cos(yaw) };
                    right = { std::cos(yaw), 0.0f, -std::sin(yaw) };
                }
            }
        }

        // 1万個のガレキを広い範囲にばらまく
        for (int i = 0; i < 10000; ++i) {
            float distFwd = Random::GeneratorFloat(10.0f, 300.0f);  // 前方奥深く
            float distRight = Random::GeneratorFloat(-150.0f, 150.0f); // 左右広く
            float height = Random::GeneratorFloat(-10.0f, 100.0f);     // 上下広く
            
            Vector3 pos = {
                spawnBase.x + forward.x * distFwd + right.x * distRight,
                spawnBase.y + height,
                spawnBase.z + forward.z * distFwd + right.z * distRight
            };
            
            int vid = virtualManager_->AddVirtualInstance(pos, {0,0,0}, {0.5f, 0.5f, 0.5f});
            if (vid >= 0) {
                DebrisAnimData anim;
                anim.baseIdleY_ = pos.y;
                anim.idleTimeY_ = Random::GeneratorFloat(0.0f, 100.0f);
                animDataList_[vid] = anim;
                activeIds_.push(vid);
            }
        }
        
        // 念のため上限を超えないようにクリップ
        while (activeIds_.size() > static_cast<size_t>(maxVirtualInstances_ - 100)) {
            int oldestId = activeIds_.front();
            activeIds_.pop();
            virtualManager_->RemoveVirtualInstance(oldestId);
        }
    }

    // --- Data-Oriented Update ---
    float dt = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (dt <= 0.0f) dt = 1.0f / 60.0f;

    auto& virtualInstances = virtualManager_->GetDenseInstances();


}

std::shared_ptr<GameObject> DebrisManagerComponent::AcquireDebris() {
    // Bossなどが要求した場合は一時的なVirtualInstanceを作って即時昇格して渡す
    int id = virtualManager_->AddVirtualInstance({0,0,0}, {0,0,0}, {0.5f, 0.5f, 0.5f});
    auto obj = virtualManager_->Promote(id);
    if (obj) {
        auto comp = obj->GetComponent<DebrisComponent>();
        if (comp) {
            comp->SetManager(this);
            comp->SetVirtualId(-1); // 使い捨て
        }
    }
    return obj;
}

void DebrisManagerComponent::ReleaseDebris(std::shared_ptr<GameObject> debris) {
    if (!debris) return;
    auto comp = debris->GetComponent<DebrisComponent>();
    if (comp) {
        int vid = comp->GetVirtualId();
        if (vid >= 0) {
            virtualManager_->Demote(vid);
        } else {
            // Bossのシールドなどで生成されたものならそのまま無効化・プール返却
            virtualManager_->ReleaseGameObject(debris);
        }
    }
}

std::shared_ptr<GameObject> DebrisManagerComponent::ExtractNearestIdleDebris(const Vector3& pos, float radius) {
    auto& virtualInstances = virtualManager_->GetDenseInstances();
    float bestDistSq = radius * radius;
    int bestId = -1;

    for (const auto& vi : virtualInstances) {
        if (!vi.isPromoted_) {
            float dx = vi.position_.x - pos.x;
            float dy = vi.position_.y - pos.y;
            float dz = vi.position_.z - pos.z;
            float distSq = dx*dx + dy*dy + dz*dz;
            if (distSq <= bestDistSq) {
                bestDistSq = distSq;
                bestId = vi.id_;
            }
        }
    }

    if (bestId >= 0) {
        auto obj = virtualManager_->Promote(bestId);
        if (obj) {
            auto comp = obj->GetComponent<DebrisComponent>();
            if (comp) {
                comp->SetVirtualId(bestId);
                comp->SetManager(this);
                comp->SetState(DebrisState::Idle);
            }
            return obj;
        }
    }
    return nullptr;
}

void DebrisManagerComponent::NotifyDestroyed(int virtualId) {
    virtualManager_->RemoveVirtualInstance(virtualId);
    // activeIds_ からの削除は不要（popされた時に既にRemove済みならEngine側で安全に無視されるため）
}
