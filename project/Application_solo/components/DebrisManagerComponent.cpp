#include "DebrisManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/SceneSerializer.h"
#include "Framework/BaseScene.h"
#include "DebrisComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Renderer/Object3D/BaseModel/BaseModel.h"
#include "Engine/Core/Math/Random/Random.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"

void DebrisManagerComponent::OnRegisterProperties() {
    RegisterProperty("Pool Size", &poolSize_);
}

void DebrisManagerComponent::Initialize() {
    // Sceneへの追加を確実に行うため、プールの生成は最初のUpdateで行う
    isPoolInitialized_ = false;
}

void DebrisManagerComponent::Update() {
    if (!isPoolInitialized_) {
        // ガレキのプレハブを生成するファクトリ関数
        auto debrisFactory = [this]() -> std::shared_ptr<GameObject> {
            auto obj = std::make_shared<GameObject>("Debris");
            obj->AddComponent<TransformComponent>();
            
            // とりあえず目視できるようにキューブをアタッチ
            auto renderer = obj->AddComponent<PrimitiveRendererComponent>();
            renderer->SetShape(PrimitiveType::Cube);
            // 少し小さめに設定
            obj->GetComponent<TransformComponent>()->scale_ = { 0.5f, 0.5f, 0.5f };
            
            obj->AddComponent<DebrisComponent>();
            
            // プール内にある間は非アクティブにしておく
            obj->SetIsActive(false);
            
            // シーンの Update 中の配列破壊を防ぐため、Manager の子オブジェクトとして登録する
            if (gameObject_) {
                gameObject_->AddChild(obj);
            }
            return obj;
        };

        pool_ = std::make_unique<ObjectPool<GameObject>>(poolSize_, debrisFactory);
        isPoolInitialized_ = true;
    }

    auto input = BaseModel::GetIrufemiEngine()->GetInputManager();
    // デバッグ用: 1キーを押したら10個ランダムな場所にスポーンさせる
    if (input->IsKeyPressed('1')) {
        // 現在のカメラの座標を取得（レール移動に合わせて前方にスポーンさせるため）
        auto activeCam = BaseModel::GetIrufemiEngine()->GetCameraManager()->GetActiveCamera();
        float baseZ = activeCam ? activeCam->GetTranslate().z : 0.0f;

        for (int i = 0; i < 10; ++i) {
            auto debris = AcquireDebris();
            if (debris) {
                auto transform = debris->GetComponent<TransformComponent>();
                // カメラから少し先～奥の範囲にばらまく
                transform->position_ = {
                    Random::GeneratorFloat(-20.0f, 20.0f),
                    Random::GeneratorFloat(-5.0f, 15.0f),
                    baseZ + Random::GeneratorFloat(30.0f, 80.0f)
                };
                auto comp = debris->GetComponent<DebrisComponent>();
                comp->Initialize();
                comp->SetState(DebrisState::Idle);
            }
        }
    }
}

std::shared_ptr<GameObject> DebrisManagerComponent::AcquireDebris() {
    if (!pool_) return nullptr;
    auto obj = pool_->Acquire();
    if (obj) {
        obj->SetIsActive(true);
    }
    return obj;
}

void DebrisManagerComponent::ReleaseDebris(std::shared_ptr<GameObject> debris) {
    if (!pool_ || !debris) return;
    debris->SetIsActive(false);
    pool_->Release(debris);
}
