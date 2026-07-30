#include "ReticleUIComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"

void ReticleUIComponent::Initialize() {
    // 初期化処理が必要であればここに記述
}

void ReticleUIComponent::Update() {
    if (!gameObject_) return;

    auto transform = gameObject_->GetComponent<TransformComponent>();
    if (!transform) return;

    // マウスカーソルの座標を取得
    auto engine = BaseModel::GetIrufemiEngine();
    auto inputManager = engine->GetInputManager();
    auto cameraManager = engine->GetCameraManager();
    if (inputManager && cameraManager && cameraManager->GetActiveCamera()) {
        Irufemi::Vector2 mousePos = inputManager->GetMousePosition();
        Irufemi::Vector2 uiPos = cameraManager->GetActiveCamera()->ScreenToUIPosition(mousePos);
        
        // Transformのポジションをマウス座標に合わせる
        // Zは一番手前（カメラの手前）などに適宜設定
        transform->SetPosition(Irufemi::Vector3{ uiPos.x, uiPos.y, 0.0f });
    }
}
