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

    // しゃがみトグルの入力処理 (Ctrlキー または ゲームパッドのL3ボタン(スティック押し込み))
    if (input->IsKeyPressed(VK_CONTROL)) {
        isCrouching_ = !isCrouching_;
    }
    if (auto gamepad = input->GetGamePad()) {
        if (gamepad->IsButtonPressed(XINPUT_GAMEPAD_LEFT_THUMB)) {
            isCrouching_ = !isCrouching_;
        }
    }

    // パッドまたはキーボードからの入力取得
    float moveX = input->GetLeftStickX();
    float moveZ = input->GetLeftStickY(); 

    if (input->IsKeyDown('W') || input->IsKeyDown(VK_UP)) moveZ += 1.0f;
    if (input->IsKeyDown('S') || input->IsKeyDown(VK_DOWN)) moveZ -= 1.0f;
    if (input->IsKeyDown('A') || input->IsKeyDown(VK_LEFT)) moveX -= 1.0f;
    if (input->IsKeyDown('D') || input->IsKeyDown(VK_RIGHT)) moveX += 1.0f;

    // 正規化
    Irufemi::Vector3 moveDir = { moveX, 0.0f, moveZ };
    float length = moveDir.Length();
    if (length > 0.01f) {
        moveDir.Normalize();
        
        // 移動処理
        Irufemi::Vector3 currentPos = transform->GetPosition();
        currentPos += moveDir * moveSpeed_ * engine->GetGameDeltaTime();
        transform->SetPosition(currentPos);

        // 進行方向への旋回処理
        // モデルがデフォルトで手前（Z-方向）を向いているため、+PI して進行方向に向かせる
        float targetAngle = atan2f(moveDir.x, moveDir.z) + Irufemi::Math::PI;
        Irufemi::Vector3 currentRot = transform->GetRotation();
        
        currentRot.y = targetAngle;
        transform->SetRotation(currentRot);
    }

    // アニメーションの遷移制御
    if (animator) {
        std::string targetAnim = "";
        
        if (length > 0.01f) {
            // 移動中
            targetAnim = isCrouching_ ? crouchWalkAnimName_ : walkAnimName_;
        } else {
            // 待機中
            targetAnim = isCrouching_ ? crouchIdleAnimName_ : idleAnimName_;
        }

        if (currentAnimState_ != targetAnim) {
            animator->Play(targetAnim, true, fadeDuration_);
            currentAnimState_ = targetAnim;
        }
    }
}

nlohmann::json CG4PlayerComponent::Serialize() {
    nlohmann::json j = nlohmann::json::object();
    j["Move Speed"] = moveSpeed_;
    j["Turn Speed"] = turnSpeed_;
    j["Fade Duration"] = fadeDuration_;
    j["Idle Anim"] = idleAnimName_;
    j["Walk Anim"] = walkAnimName_;
    j["Crouch Idle Anim"] = crouchIdleAnimName_;
    j["Crouch Walk Anim"] = crouchWalkAnimName_;
    return j;
}

void CG4PlayerComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("Move Speed")) moveSpeed_ = j["Move Speed"].get<float>();
    if (j.contains("Turn Speed")) turnSpeed_ = j["Turn Speed"].get<float>();
    if (j.contains("Fade Duration")) fadeDuration_ = j["Fade Duration"].get<float>();
    if (j.contains("Idle Anim")) idleAnimName_ = j["Idle Anim"].get<std::string>();
    if (j.contains("Walk Anim")) walkAnimName_ = j["Walk Anim"].get<std::string>();
    if (j.contains("Crouch Idle Anim")) crouchIdleAnimName_ = j["Crouch Idle Anim"].get<std::string>();
    if (j.contains("Crouch Walk Anim")) crouchWalkAnimName_ = j["Crouch Walk Anim"].get<std::string>();
}

void CG4PlayerComponent::OnRegisterProperties() {
    RegisterProperty("Move Speed", &moveSpeed_);
    RegisterProperty("Turn Speed", &turnSpeed_);
    RegisterProperty("Fade Duration", &fadeDuration_);
    RegisterProperty("Idle Anim", &idleAnimName_);
    RegisterProperty("Walk Anim", &walkAnimName_);
    RegisterProperty("Crouch Idle Anim", &crouchIdleAnimName_);
    RegisterProperty("Crouch Walk Anim", &crouchWalkAnimName_);
}
