#include "OrbitCameraController.h"
#include "Engine/Core/Math/MathFunction.h"
#include <windows.h> // VK_LSHIFT, VK_RSHIFT
#include <algorithm>

void OrbitCameraController::UpdateCameraInput(Camera* camera, InputManager* input) {
    if (!camera || !input) return;

    bool isMiddleButtonDown = input->IsMouseButtonDown(Mouse::Button::Middle);
    auto* keyboard = input->GetKeyboard();
    bool isShiftDown = keyboard->IsKeyDown(VK_LSHIFT) || keyboard->IsKeyDown(VK_RSHIFT);
    Irufemi::Vector2 mouseDelta = input->GetMouseDelta();
    
    // 初期化されていない場合は現在のカメラからTarget等を逆算する
    if (!isInitialized_) {
        // カメラが現在いる位置から原点 (0,0,0) までの距離を基準に Target を算出する
        float distToOrigin = Irufemi::Math::Length(camera->GetTranslate());
        // 距離が近すぎる場合は最低限の距離を保つ
        if (distToOrigin < 1.0f) distToOrigin = 10.0f;
        SyncTargetFromCamera(camera, distToOrigin);
    } else {
        // 初期化済みだが、外部要因（シーンロードやスクリプト等）でカメラが移動した場合を検知
        float posDiff = Irufemi::Math::Length(Irufemi::Math::Subtract(camera->GetTranslate(), lastCameraPosition_));
        float rotDiff = Irufemi::Math::Length(Irufemi::Math::Subtract(camera->GetRotate(), lastCameraRotation_));
        if (posDiff > 0.01f || rotDiff > 0.01f) {
            float distToOrigin = Irufemi::Math::Length(camera->GetTranslate());
            if (distToOrigin < 1.0f) distToOrigin = 10.0f;
            SyncTargetFromCamera(camera, distToOrigin);
        }
    }
    
    bool cameraChanged = false;
    
    if (isMiddleButtonDown) {
        if (isShiftDown) {
            // パン操作 (Shift + 中ボタンドラッグ)
            const float panSpeed = 0.05f;
            Irufemi::Matrix4x4 viewInverse = Irufemi::Math::Inverse(camera->GetViewMatrix());
            Irufemi::Vector3 right = { viewInverse.m[0][0], viewInverse.m[0][1], viewInverse.m[0][2] };
            Irufemi::Vector3 up = { viewInverse.m[1][0], viewInverse.m[1][1], viewInverse.m[1][2] };
            target_ = Irufemi::Math::Add(target_, Irufemi::Math::Multiply(-panSpeed * mouseDelta.x, right));
            target_ = Irufemi::Math::Add(target_, Irufemi::Math::Multiply(panSpeed * mouseDelta.y, up));
            cameraChanged = true;
        } else {
            // オービット操作 (中ボタンドラッグ)
            const float rotationSpeed = 0.005f;
            Irufemi::Vector3 rotate = camera->GetRotate();
            rotate.y += mouseDelta.x * rotationSpeed;
            rotate.x += mouseDelta.y * rotationSpeed;
            // X軸回転を制限
            rotate.x = std::clamp(rotate.x, -Irufemi::Math::PIDiv2, Irufemi::Math::PIDiv2);
            camera->SetRotate(rotate);
            cameraChanged = true;
        }
    }
    
    // ズーム操作 (マウスホイール)
    float wheelDelta = input->GetMouseWheelDelta();
    if (wheelDelta != 0.0f) {
        const float zoomSpeed = 2.0f;
        distance_ -= wheelDelta * zoomSpeed;
        if (distance_ < 1.0f) distance_ = 1.0f; // 最小距離制限
        cameraChanged = true;
    }
    
    // カメラの位置を更新
    if (cameraChanged || isMiddleButtonDown) {
        Irufemi::Vector3 rotate = camera->GetRotate();
        Irufemi::Matrix4x4 rotMat = Irufemi::Math::MakeRotateXYZMatrix(rotate);
        Irufemi::Vector3 offset = { 0.0f, 0.0f, -distance_ };
        offset = Irufemi::Math::TransformNormal(offset, rotMat);
        camera->SetTranslate(Irufemi::Math::Add(target_, offset));
        camera->UpdateMatrix();
    }
    
    // 現在のカメラ状態を記憶（自身で動かした結果を含む）
    lastCameraPosition_ = camera->GetTranslate();
    lastCameraRotation_ = camera->GetRotate();
}

void OrbitCameraController::Focus(Camera* camera, const Irufemi::Vector3& targetPosition, float distance) {
    if (!camera) return;
    
    target_ = targetPosition;
    if (distance > 0.0f) {
        distance_ = distance;
    }
    
    // 直ちにカメラ位置を更新
    Irufemi::Vector3 rotate = camera->GetRotate();
    Irufemi::Matrix4x4 rotMat = Irufemi::Math::MakeRotateXYZMatrix(rotate);
    Irufemi::Vector3 offset = { 0.0f, 0.0f, -distance_ };
    offset = Irufemi::Math::TransformNormal(offset, rotMat);
    camera->SetTranslate(Irufemi::Math::Add(target_, offset));
    camera->UpdateMatrix();
    
    isInitialized_ = true;
    lastCameraPosition_ = camera->GetTranslate();
    lastCameraRotation_ = camera->GetRotate();
}

void OrbitCameraController::SyncTargetFromCamera(const Camera* camera, float distance) {
    if (!camera) return;
    
    distance_ = distance;
    
    Irufemi::Matrix4x4 rotMat = Irufemi::Math::MakeRotateXYZMatrix(camera->GetRotate());
    Irufemi::Vector3 offset = { 0.0f, 0.0f, -distance_ };
    offset = Irufemi::Math::TransformNormal(offset, rotMat);
    
    target_ = Irufemi::Math::Subtract(camera->GetTranslate(), offset);
    
    isInitialized_ = true;
    lastCameraPosition_ = camera->GetTranslate();
    lastCameraRotation_ = camera->GetRotate();
}

void OrbitCameraController::SetPreset(Preset preset, Camera* camera) {
    if (!camera) return;

    switch (preset) {
    case Preset::TopDown:
        camera->SetRotate({ Irufemi::Math::PIDiv2, 0.0f, 0.0f });
        break;
    case Preset::Diagonal:
        camera->SetRotate({ 0.6f, 0.78f, 0.0f }); // 約35度見下ろし、45度回転
        break;
    case Preset::Front:
        camera->SetRotate({ 0.0f, 0.0f, 0.0f });
        break;
    }

    // Preset適用後、Targetは動かさずに位置を更新する
    Irufemi::Vector3 rotate = camera->GetRotate();
    Irufemi::Matrix4x4 rotMat = Irufemi::Math::MakeRotateXYZMatrix(rotate);
    Irufemi::Vector3 offset = { 0.0f, 0.0f, -distance_ };
    offset = Irufemi::Math::TransformNormal(offset, rotMat);
    camera->SetTranslate(Irufemi::Math::Add(target_, offset));
    camera->UpdateMatrix();

    isInitialized_ = true;
    lastCameraPosition_ = camera->GetTranslate();
    lastCameraRotation_ = camera->GetRotate();
}
