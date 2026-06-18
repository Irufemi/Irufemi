#pragma once
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Engine/Core/Math/Vector3.h"

class EditorCameraController {
public:
    void UpdateCameraInput(Camera* camera, InputManager* input);
    
    /**
     * @brief 指定したワールド座標をカメラの中心に収めるよう移動させます
     * @param camera カメラオブジェクト
     * @param targetPosition フォーカスする対象の座標
     */
    void Focus(Camera* camera, const Vector3& targetPosition);

private:
    bool isInitialized_ = false;
    Vector3 target_ = {0.0f, 0.0f, 0.0f};
    float distance_ = 50.0f;
};
