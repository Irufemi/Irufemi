#include "DroneComponent.h"
#include "BossComponent.h"
#include "BossBulletManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/Math/Random/Random.h"

DroneComponent::DroneComponent() {}

void DroneComponent::OnRegisterProperties() {
    RegisterProperty("Orbit Radius", &orbitRadius_);
    RegisterProperty("Orbit Speed", &orbitSpeed_);
    RegisterProperty("Fire Interval", &fireInterval_);
    
    // デバッグ用
    RegisterProperty("Has Player", &hasPlayer_);
    RegisterProperty("Player Pos X", &debugPlayerPos_.x);
    RegisterProperty("Player Pos Y", &debugPlayerPos_.y);
    RegisterProperty("Player Pos Z", &debugPlayerPos_.z);
}

void DroneComponent::SetOrbit(std::weak_ptr<GameObject> boss, float radius, float initialAngle, float speed) {
    boss_ = boss;
    orbitRadius_ = radius;
    orbitAngle_ = initialAngle;
    orbitSpeed_ = speed;
    hp_ = 1.0f;

    // 発射タイミングをドローンごとにずらす
    fireTimer_ = Random::GeneratorFloat(0.0f, fireInterval_);
    
    if (auto scene = GetGameObject()->GetScene()) {
        // IrufemiEngineのFindGameObjectはルートオブジェクトしか探せないため、親から子を探す
        auto playerCart = scene->FindGameObject("PlayerCart");
        if (playerCart) {
            for (auto& child : playerCart->GetChildren()) {
                if (child && child->GetName() == "Player") {
                    player_ = child;
                    break;
                }
            }
        }
        
        // もし見つからなければ、直接探す（念のため）
        if (!player_.lock()) {
            player_ = scene->FindGameObject("Player");
        }
    }
}

void DroneComponent::TakeDamage(float damage) {
    hp_ -= damage;
    if (hp_ <= 0.0f) {
        // TODO: 破壊エフェクト・SEの再生
        GetGameObject()->SetIsActive(false);
    }
}

void DroneComponent::Update() {
    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();

    if (auto boss = boss_.lock()) {
        auto bossTransform = boss->GetComponent<TransformComponent>();
        auto transform = GetGameObject()->GetComponent<TransformComponent>();

        if (bossTransform && transform) {
            // ボスを中心に公転
            orbitAngle_ += orbitSpeed_ * deltaTime;
            
            // XY平面での円運動（Z軸周り）
            Vector3 bossPos = bossTransform->GetWorldPosition();
            float x = std::cos(orbitAngle_) * orbitRadius_;
            float y = std::sin(orbitAngle_) * orbitRadius_;
            
            // ドローンの座標を設定
            Vector3 targetPos = bossPos + Vector3{x, y, 0.0f};
            transform->SetPosition(targetPos);
            
            // 常にプレイヤー（自機）の方を向かせる
            if (auto player = player_.lock()) {
                hasPlayer_ = true;
                if (auto playerTransform = player->GetComponent<TransformComponent>()) {
                    Vector3 playerPos = playerTransform->GetWorldPosition();
                    debugPlayerPos_ = playerPos; // インスペクター確認用
                    Vector3 dirToPlayer = Math::Subtract(playerPos, targetPos).GetNormalized();
                    
                    // Assimp/BlenderによるZ軸反転ズレを相殺するためベクトルを反転
                    dirToPlayer = Math::Multiply(-1.0f, dirToPlayer);
                    
                    transform->SetRotation(Math::LookRotation(dirToPlayer));
                }
            } else {
                hasPlayer_ = false;
                // プレイヤーがいない場合はボスの方向を向く
                Vector3 dirToBoss = Math::Subtract(bossPos, targetPos).GetNormalized();
                dirToBoss = Math::Multiply(-1.0f, dirToBoss);
                transform->SetRotation(Math::LookRotation(dirToBoss));
            }
        }

        // 弾の発射ロジック
        if (bulletManager_) {
            fireTimer_ -= deltaTime;
            if (fireTimer_ <= 0.0f) {
                FireBullet();
                fireTimer_ = fireInterval_ + Random::GeneratorFloat(-0.5f, 0.5f);
            }
        }
    }
}

void DroneComponent::FireBullet() {
    auto transform = GetGameObject()->GetComponent<TransformComponent>();
    if (!transform || !bulletManager_) return;

    Vector3 myPos = transform->GetWorldPosition();
    Vector3 forward;
    
    // プレイヤーの方向に向けて撃つ
    if (auto player = player_.lock()) {
        if (auto playerTransform = player->GetComponent<TransformComponent>()) {
            Vector3 playerPos = playerTransform->GetWorldPosition();
            // プレイヤーの中心付近（少し上）を狙う
            playerPos.y += 1.0f;
            forward = Math::Subtract(playerPos, myPos).GetNormalized();
        }
    } else {
        forward = Math::TransformNormal(Vector3::forward, Math::MakeRotateXYZMatrix(transform->GetRotation()));
    }
    
    // 発射速度
    Vector3 velocity = forward * 20.0f; 
    bulletManager_->SpawnBullet(myPos, velocity);
}
