#include "CG4PlayerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Logic/AnimatorComponent.h"
#include "Framework/BaseScene.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Engine/Core/Math/MathFunction.h"
#include <cmath>

CG4PlayerComponent::CG4PlayerComponent() = default;
CG4PlayerComponent::~CG4PlayerComponent() = default;

void CG4PlayerComponent::Initialize() {
}

void CG4PlayerComponent::Update() {
    auto gameObject = GetGameObject();
    if (!gameObject) return;

    auto transform = gameObject->GetComponent<TransformComponent>();
    auto animator = gameObject->GetComponent<AnimatorComponent>();
    if (!transform) return;

    auto scene = gameObject->GetScene();
    if (!scene) return;

    auto engine = scene->GetEngine();
    if (!engine) return;

    auto input = engine->GetInputManager();
    if (!input) return;

    // パッドまたはキーボードからの入力取得
    float moveX = input->GetLeftStickX();
    float moveZ = input->GetLeftStickY(); 

    if (input->IsKeyDown('W') || input->IsKeyDown(VK_UP)) moveZ += 1.0f;
    if (input->IsKeyDown('S') || input->IsKeyDown(VK_DOWN)) moveZ -= 1.0f;
    if (input->IsKeyDown('A') || input->IsKeyDown(VK_LEFT)) moveX -= 1.0f;
    if (input->IsKeyDown('D') || input->IsKeyDown(VK_RIGHT)) moveX += 1.0f;

    // 正規化
    Vector3 moveDir = { moveX, 0.0f, moveZ };
    float length = moveDir.Length();
    if (length > 0.01f) {
        moveDir.Normalize();
        
        // 移動処理
        Vector3 currentPos = transform->GetPosition();
        currentPos += moveDir * moveSpeed_ * engine->GetGameDeltaTime();
        transform->SetPosition(currentPos);

        // 進行方向への旋回処理
        // モデルがデフォルトで手前（Z-方向）を向いているため、+PI して進行方向に向かせる
        float targetAngle = atan2f(moveDir.x, moveDir.z) + Math::PI;
        Vector3 currentRot = transform->GetRotation();
        
        currentRot.y = targetAngle;
        transform->SetRotation(currentRot);

        // 動いているときは歩きアニメーションなどを再生したい場合はここに記述
        // if (animator) {
        //     // animator->Play("sample/walk.gltf", true); // すでに再生されていれば何もしない等の処理が必要
        // }
    } else {
        // 止まっているときのアニメーション遷移など
    }
}

nlohmann::json CG4PlayerComponent::Serialize() {
    nlohmann::json j = nlohmann::json::object();
    j["Move Speed"] = moveSpeed_;
    j["Turn Speed"] = turnSpeed_;
    return j;
}

void CG4PlayerComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("Move Speed")) moveSpeed_ = j["Move Speed"].get<float>();
    if (j.contains("Turn Speed")) turnSpeed_ = j["Turn Speed"].get<float>();
}

void CG4PlayerComponent::OnRegisterProperties() {
    RegisterProperty("Move Speed", &moveSpeed_);
    RegisterProperty("Turn Speed", &turnSpeed_);
}
