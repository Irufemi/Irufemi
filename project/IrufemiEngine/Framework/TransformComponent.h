#pragma once
#include "Component.h"
#include "Engine/Core/Math/Vector3.h"

class TransformComponent : public Component {
public:
    Vector3 position_ = { 0.0f, 0.0f, 0.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f }; // Euler angles in radians
    Vector3 scale_ = { 1.0f, 1.0f, 1.0f };

    void Initialize() override {}
    void Update() override {}

#ifdef EditorMode
    void OnInspectorGUI() override;
#endif
};
