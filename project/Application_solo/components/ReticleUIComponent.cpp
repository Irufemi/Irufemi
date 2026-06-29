#include "ReticleUIComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"

void ReticleUIComponent::Initialize() {
    // 初期化処理が必要であればここに記述
}

void ReticleUIComponent::Update() {
    if (!gameObject_) return;

    auto transform = gameObject_->GetComponent<TransformComponent>();
    if (!transform) return;

    // マウスカーソルの座標を取得
    auto inputManager = BaseModel::GetIrufemiEngine()->GetInputManager();
    if (inputManager) {
        Vector2 mousePos = inputManager->GetMousePosition();
        
        // Transformのポジションをマウス座標に合わせる
        // Zは一番手前（カメラの手前）などに適宜設定
        transform->SetPosition(Vector3{ mousePos.x, mousePos.y, 0.0f });
    }
}
