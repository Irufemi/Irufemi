#include "OrbitCameraController.h"
#include "Engine/Core/Math/MathFunction.h"
#include <windows.h> // VK_LSHIFT, VK_RSHIFT
#include <algorithm>

void OrbitCameraController::UpdateCameraInput(Camera* camera, InputManager* input) {
    if (!camera || !input) return;

    bool isMiddleButtonDown = input->IsMouseButtonDown(Mouse::Button::Middle);
    auto* keyboard = input->GetKeyboard();
    bool isShiftDown = keyboard->IsKeyDown(VK_LSHIFT) || keyboard->IsKeyDown(VK_RSHIFT);
    Vector2 mouseDelta = input->GetMouseDelta();
    
    // 初期化されていない場合は現在のカメラからTarget等を逆算する
    if (!isInitialized_) {
        // カメラが現在いる位置から原点 (0,0,0) までの距離を基準に Target を算出する
        float distToOrigin = Math::Length(camera->GetTranslate());
        // 距離が近すぎる場合は最低限の距離を保つ
        if (distToOrigin < 1.0f) distToOrigin = 10.0f;
        SyncTargetFromCamera(camera, distToOrigin);
    } else {
        // 初期化済みだが、外部要因（シーンロードやスクリプト等）でカメラが移動した場合を検知
        float posDiff = Math::Length(Math::Subtract(camera->GetTranslate(), lastCameraPosition_));
        float rotDiff = Math::Length(Math::Subtract(camera->GetRotate(), lastCameraRotation_));
        if (posDiff > 0.01f || rotDiff > 0.01f) {
            float distToOrigin = Math::Length(camera->GetTranslate());
            if (distToOrigin < 1.0f) distToOrigin = 10.0f;
            SyncTargetFromCamera(camera, distToOrigin);
        }
    }
    
    bool cameraChanged = false;
    
    if (isMiddleButtonDown) {
        if (isShiftDown) {
            // パン操作 (Shift + 中ボタンドラッグ)
            const float panSpeed = 0.05f;
            Matrix4x4 viewInverse = Math::Inverse(camera->GetViewMatrix());
            Vector3 right = { viewInverse.m[0][0], viewInverse.m[0][1], viewInverse.m[0][2] };
            Vector3 up = { viewInverse.m[1][0], viewInverse.m[1][1], viewInverse.m[1][2] };
            target_ = Math::Add(target_, Math::Multiply(-panSpeed * mouseDelta.x, right));
            target_ = Math::Add(target_, Math::Multiply(panSpeed * mouseDelta.y, up));
            cameraChanged = true;
        } else {
            // オービット操作 (中ボタンドラッグ)
            const float rotationSpeed = 0.005f;
            Vector3 rotate = camera->GetRotate();
            rotate.y += mouseDelta.x * rotationSpeed;
            rotate.x += mouseDelta.y * rotationSpeed;
            // X軸回転を制限
            rotate.x = std::clamp(rotate.x, -Math::PIDiv2, Math::PIDiv2);
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
        Vector3 rotate = camera->GetRotate();
        Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(rotate);
        Vector3 offset = { 0.0f, 0.0f, -distance_ };
        offset = Math::TransformNormal(offset, rotMat);
        camera->SetTranslate(Math::Add(target_, offset));
        camera->UpdateMatrix();
    }
    
    // 現在のカメラ状態を記憶（自身で動かした結果を含む）
    lastCameraPosition_ = camera->GetTranslate();
    lastCameraRotation_ = camera->GetRotate();
}

void OrbitCameraController::Focus(Camera* camera, const Vector3& targetPosition, float distance) {
    if (!camera) return;
    
    target_ = targetPosition;
    if (distance > 0.0f) {
        distance_ = distance;
    }
    
    // 直ちにカメラ位置を更新
    Vector3 rotate = camera->GetRotate();
    Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(rotate);
    Vector3 offset = { 0.0f, 0.0f, -distance_ };
    offset = Math::TransformNormal(offset, rotMat);
    camera->SetTranslate(Math::Add(target_, offset));
    camera->UpdateMatrix();
    
    isInitialized_ = true;
    lastCameraPosition_ = camera->GetTranslate();
    lastCameraRotation_ = camera->GetRotate();
}

void OrbitCameraController::SyncTargetFromCamera(const Camera* camera, float distance) {
    if (!camera) return;
    
    distance_ = distance;
    
    Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(camera->GetRotate());
    Vector3 offset = { 0.0f, 0.0f, -distance_ };
    offset = Math::TransformNormal(offset, rotMat);
    
    target_ = Math::Subtract(camera->GetTranslate(), offset);
    
    isInitialized_ = true;
    lastCameraPosition_ = camera->GetTranslate();
    lastCameraRotation_ = camera->GetRotate();
}

void OrbitCameraController::SetPreset(Preset preset, Camera* camera) {
    if (!camera) return;

    switch (preset) {
    case Preset::TopDown:
        camera->SetRotate({ Math::PIDiv2, 0.0f, 0.0f });
        break;
    case Preset::Diagonal:
        camera->SetRotate({ 0.6f, 0.78f, 0.0f }); // 約35度見下ろし、45度回転
        break;
    case Preset::Front:
        camera->SetRotate({ 0.0f, 0.0f, 0.0f });
        break;
    }

    // Preset適用後、Targetは動かさずに位置を更新する
    Vector3 rotate = camera->GetRotate();
    Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(rotate);
    Vector3 offset = { 0.0f, 0.0f, -distance_ };
    offset = Math::TransformNormal(offset, rotMat);
    camera->SetTranslate(Math::Add(target_, offset));
    camera->UpdateMatrix();

    isInitialized_ = true;
    lastCameraPosition_ = camera->GetTranslate();
    lastCameraRotation_ = camera->GetRotate();
}
