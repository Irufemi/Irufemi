#include "RailMechanics/RailShooterEnemyComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h" // 描画オンオフ用
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Player/TargetableComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/Camera/CameraShakeComponent.h"
#include "Engine/Manager/CollisionManager.h"

void RailShooterEnemyComponent::OnRegisterProperties() {
    RegisterProperty("SpawnProgress", &spawnProgress_);
    RegisterProperty("Speed", &speed_);
    RegisterProperty("HP", &hp_);
}

void RailShooterEnemyComponent::Initialize() {
    hp_ = 100; // プール再利用時にHPを回復する
    isActive_ = false;

    auto targetable = gameObject_->GetComponent<TargetableComponent>();
    if (!targetable) {
        targetable = gameObject_->AddComponent<TargetableComponent>().get();
        targetable->Initialize();
    }
    
    // 初期状態では非表示にしておく
    if (gameObject_) {
        if (auto renderer = gameObject_->GetComponent<MeshRendererComponent>()) {
            // (仮) 初期状態は非アクティブとして振る舞う
            // エンジン側に Enable/Disable の機能があればそれを使用する
            // ここでは簡易的にスケールを 0 にして見えないようにするか、Rendererを調整する
        }

        auto collider = gameObject_->GetComponent<SphereColliderComponent>();
        if (!collider) {
            collider = gameObject_->AddComponent<SphereColliderComponent>().get();
            collider->Initialize();
        }
        if (collider) {
            collider->isTrigger_ = true;
            collider->SetLocalRadius(1.5f);
            
            auto cm = BaseModel::GetIrufemiEngine()->GetCollisionManager();
            if (cm) {
                collider->layer_ = cm->GetLayerMask("Enemy");
            }
        }
    }
}

void RailShooterEnemyComponent::Update() {
    if (!gameObject_) return;

    // TODO: プレイヤーの進行度をグローバルまたはManagerから取得して比較
    // ここでは単純に isActive になったら前に進むだけの仮実装
    if (isActive_) {
        // エンジンから正確なゲーム内時間差を取得
        float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
        if (deltaTime <= 0.0f) {
            return;
        } 
        if (auto transform = GetTransform()) {
            // 前方(Z軸正方向など)に進む
            // Transformの rotation_ を元に向きベクトルを計算して足す
            float yaw = transform->GetRotation().y;
            Irufemi::Vector3 forward = { std::sin(yaw), 0.0f, std::cos(yaw) };
            
            Irufemi::Vector3 pos = transform->GetPosition();
            pos.x += forward.x * speed_ * deltaTime;
            pos.y += forward.y * speed_ * deltaTime;
            pos.z += forward.z * speed_ * deltaTime;
            transform->SetPosition(pos);
        }
    } else {
        // 条件を満たしたらアクティブ化
        // (仮: 常にアクティブにする)
        isActive_ = true;
    }
}

void RailShooterEnemyComponent::TakeDamage(int damage) {
    if (!IsAlive()) return;



    hp_ -= damage;
    if (hp_ <= 0) {
        hp_ = 0;
        isActive_ = false;
        if (gameObject_) {
            if (onDeathCallback_) {
                onDeathCallback_(gameObject_);
            } else {
                gameObject_->SetIsActive(false); // 表示・更新を停止
                gameObject_->Destroy();          // シーンから完全に削除する
            }
        }
    }
}
