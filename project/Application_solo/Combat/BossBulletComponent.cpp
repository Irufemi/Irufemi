#include "Combat/BossBulletComponent.h"
#include "Combat/BossBulletManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Core/Math/MathFunction.h"

BossBulletComponent::BossBulletComponent() {}

void BossBulletComponent::OnRegisterProperties() {
    // 登録が必要なプロパティがあれば記述
}

std::shared_ptr<Component> BossBulletComponent::Clone() {
    auto clone = std::make_shared<BossBulletComponent>();
    clone->CopyPropertiesFrom(this);
    clone->manager_ = this->manager_;
    clone->velocity_ = this->velocity_;
    clone->lifeTimer_ = this->lifeTimer_;
    clone->isActiveBullet_ = this->isActiveBullet_;
    return clone;
}

void BossBulletComponent::Shoot(BossBulletManagerComponent* manager, const Irufemi::Vector3& startPos, const Irufemi::Vector3& velocity, float lifeTime) {
    manager_ = manager;
    velocity_ = velocity;
    lifeTimer_ = lifeTime;
    isActiveBullet_ = true;

    auto transform = GetTransform();
    if (transform) {
        transform->SetWorldPosition(startPos);
        // velocityの方向を向かせる処理（必要であれば）
        if (velocity.LengthSquared() > 0.001f) {
            Irufemi::Matrix4x4 rotMat = Irufemi::Math::DirectionToDirection(Irufemi::Vector3{0, 0, 1}, velocity.GetNormalized());
            transform->SetWorldRotation(Irufemi::Math::ExtractEulerFromMatrix(rotMat));
        }
    }
}

void BossBulletComponent::Update() {
    if (!isActiveBullet_) return;

    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();

    // 移動処理
    auto transform = GetTransform();
    if (transform) {
        Irufemi::Vector3 pos = transform->GetWorldPosition();
        pos += velocity_ * deltaTime;
        transform->SetWorldPosition(pos);
    }

    // 寿命管理
    lifeTimer_ -= deltaTime;
    if (lifeTimer_ <= 0.0f) {
        OnIntercepted(); // 寿命切れでも一旦マネージャーに返す
    }
}

void BossBulletComponent::OnIntercepted() {
    if (!isActiveBullet_) return;
    isActiveBullet_ = false;

    // TODO: ここで放電エフェクトやパーティクル（VoxelParticleなど）を再生する
    
    // マネージャーに自身を返却してプールへ戻す
    // Manager がプール方式ではなくなったため、BossBulletComponent 自体を今後使わない想定です。
    // （互換性のため残していますが、ReleaseBullet は呼び出しません）
    // if (manager_) {
    //     manager_->ReleaseBullet(GetGameObject());
    // } else {
    // マネージャーがいない場合は直接非アクティブ化
    GetGameObject()->SetIsActive(false);
}
