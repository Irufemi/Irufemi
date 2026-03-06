#define NOMINMAX
#include "DebugCamera.h"
#include "engine/Input/Mouse.h"
#include "engine/Input/Keyboard.h"
#include "function/Math.h"
#include <algorithm>
#include <string>

void DebugCamera::Initialize(InputManager* input, int windowWidth, int windowHeight) {
    input_ = input;
    camera_.Initialize(windowWidth, windowHeight);
    // 初期回転と位置を計算
    Vector3 rotate = camera_.GetRotate();
    Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(rotate);
    Vector3 offset = { 0.0f, 0.0f, -distance_ };
    offset = Math::TransformNormal(offset, rotMat);
    camera_.SetTranslate(Math::Add(target_, offset));
    camera_.UpdateMatrix();
}

void DebugCamera::Update() {
    // ImGuiでの操作を先に反映させる
    camera_.Update("DebugCamera");

    Mouse* mouse = input_->GetMouse();
    Keyboard* keyboard = input_->GetKeyboard();

    bool isMiddleButtonDown = mouse->IsButtonDown(Mouse::Button::Middle);
    bool isShiftDown = keyboard->IsKeyDown(VK_LSHIFT) || keyboard->IsKeyDown(VK_RSHIFT);
    Vector2 mouseDelta = mouse->GetDelta();

    // Blenderライクな操作
    if (isMiddleButtonDown) {
        if (isShiftDown) {
            // パン操作 (Shift + 中ボタンドラッグ)
            const float panSpeed = 0.05f;
            Matrix4x4 viewInverse = Math::Inverse(camera_.GetViewMatrix());
            Vector3 right = { viewInverse.m[0][0], viewInverse.m[0][1], viewInverse.m[0][2] };
            Vector3 up = { viewInverse.m[1][0], viewInverse.m[1][1], viewInverse.m[1][2] };
            target_ = Math::Add(target_, Math::Multiply(-panSpeed * mouseDelta.x, right));
            target_ = Math::Add(target_, Math::Multiply(panSpeed * mouseDelta.y, up));
        }
        else {
            // オービット操作 (中ボタンドラッグ)
            const float rotationSpeed = 0.005f;
            Vector3 rotate = camera_.GetRotate();
            rotate.y += mouseDelta.x * rotationSpeed;
            rotate.x += mouseDelta.y * rotationSpeed;
            // X軸回転を制限
            rotate.x = std::clamp(rotate.x, -Math::PIDiv2, Math::PIDiv2);
            camera_.SetRotate(rotate);
        }
    }

    // ズーム操作 (マウスホイール)
    float wheelDelta = mouse->GetWheelDelta();
    if (wheelDelta != 0.0f) {
        // --- デバッグコード追加 ---
        std::string dbgMsg = "[DebugCamera] GetWheelDelta: " + std::to_string(wheelDelta) + ", old distance: " + std::to_string(distance_) + "\n";
        OutputDebugStringA(dbgMsg.c_str());
        // -------------------------

        const float zoomSpeed = 2.0f;
        distance_ -= wheelDelta * zoomSpeed;
        distance_ = std::max(distance_, 1.0f); // 最小距離制限

        // --- デバッグコード追加 ---
        dbgMsg = "[DebugCamera] new distance: " + std::to_string(distance_) + "\n";
        OutputDebugStringA(dbgMsg.c_str());
        // -------------------------
    }

    // カメラの位置を更新
    Vector3 rotate = camera_.GetRotate();
    Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(rotate);
    Vector3 offset = { 0.0f, 0.0f, -distance_ };
    offset = Math::TransformNormal(offset, rotMat);
    camera_.SetTranslate(Math::Add(target_, offset));

    // マウス操作後の最終的な行列を更新
    camera_.UpdateMatrix();
}