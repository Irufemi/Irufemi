#include "CameraComponent.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"

CameraComponent::CameraComponent() = default;
CameraComponent::~CameraComponent() = default;

void CameraComponent::OnRegisterProperties() {
    RegisterProperty("FOV", &fovAngleY_);
    RegisterProperty("NearZ", &nearZ_);
    RegisterProperty("FarZ", &farZ_);
    RegisterProperty("MakeActive", &makeActive_);
}

void CameraComponent::Initialize() {
    camera_ = std::make_shared<Camera>();
    
    auto* engine = BaseModel::GetIrufemiEngine();
    int clientWidth = engine ? engine->GetClientWidth() : 1280;
    int clientHeight = engine ? engine->GetClientHeight() : 720;
    camera_->Initialize(clientWidth, clientHeight);
    
    // プロパティ値を適用
    camera_->SetFovY(fovAngleY_);
    camera_->SetFarClip(farZ_);
    
    // カメラマネージャに登録
    if (gameObject_) {
        if (engine && engine->GetCameraManager()) {
            engine->GetCameraManager()->AddCamera(gameObject_->GetName(), camera_);
            if (makeActive_) {
                engine->GetCameraManager()->SetActiveCamera(gameObject_->GetName());
            }
        }
    }
}

void CameraComponent::Update() {
    if (!gameObject_ || !camera_) return;

    auto transform = GetTransform();
    if (!transform) return;

    // GameObjectのTransformとCameraの座標・角度を同期（ワールド座標系）＋ 演出オフセット
    camera_->SetTranslate(Irufemi::Math::Add(transform->GetWorldPosition(), positionOffset_));
    camera_->SetRotate(Irufemi::Math::Add(transform->GetWorldRotation(), rotationOffset_));
    
    // パラメータの動的更新反映
    camera_->SetFovY(fovAngleY_);
    camera_->SetFarClip(farZ_);
    
    camera_->UpdateMatrix();
}
