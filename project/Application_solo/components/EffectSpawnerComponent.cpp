#include "EffectSpawnerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"

void EffectSpawnerComponent::OnRegisterProperties() {
    RegisterProperty("Prefab Path", &prefabPath_);
    RegisterProperty("Position Offset", &positionOffset_);
    RegisterProperty("Base Rotation", &baseRotation_);
    RegisterProperty("Base Scale", &baseScale_);
    RegisterProperty("Attach To Target", &attachToTarget_);
}

void EffectSpawnerComponent::PlayEffect(const Vector3& hitPosition, GameObject* target) {
    if (!gameObject_ || prefabPath_.empty()) return;

    Vector3 finalPosition = {
        hitPosition.x + positionOffset_.x,
        hitPosition.y + positionOffset_.y,
        hitPosition.z + positionOffset_.z
    };

    // Prefabからインスタンスを生成
    auto spawnedObj = gameObject_->Instantiate(prefabPath_, finalPosition);

    if (spawnedObj) {
        // Transformに初期パラメータを適用
        if (auto transform = spawnedObj->GetComponent<TransformComponent>()) {
            transform->SetRotation(baseRotation_);
            transform->SetScale(baseScale_);
        }

        // オプション：ターゲットの子オブジェクトにする（追従させる）
        if (attachToTarget_ && target) {
            target->AddChild(spawnedObj);
        }
    }
}
