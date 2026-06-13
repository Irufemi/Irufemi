#pragma once
#include "Component.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/System/ComponentPool.h"

class TransformComponent : public Component {
public:
    Vector3 position_ = { 0.0f, 0.0f, 0.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f }; // Euler angles in radians
    Vector3 scale_ = { 1.0f, 1.0f, 1.0f };

    Vector3 worldPosition_ = { 0.0f, 0.0f, 0.0f };
    Vector3 worldRotation_ = { 0.0f, 0.0f, 0.0f };
    Vector3 worldScale_ = { 1.0f, 1.0f, 1.0f };

    void Initialize() override {}
    void Update() override; // 一括更新に移行するため、個別のUpdateは空にする
    
    /**
     * @brief 自身の行列を計算する（依存解決付き）
     */
    void ComputeMatrix();

    /**
     * @brief プール上の全Transformを一括更新する（DOD）
     */
    static void UpdateAll();

    bool CanUpdateInEditMode() const override { return true; }

    const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
    const Matrix4x4& GetLocalMatrix() const { return localMatrix_; }

    std::string GetComponentName() const override { return "TransformComponent"; }
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

#ifdef EditorMode

#endif

private:
    Matrix4x4 localMatrix_ = Math::MakeIdentity4x4();
    Matrix4x4 worldMatrix_ = Math::MakeIdentity4x4();

    uint32_t lastUpdateFrame_ = 0;
    static inline uint32_t currentFrame_ = 1;
};

// ComponentPool 対応を宣言
template<> struct IsPooledComponent<TransformComponent> : std::true_type {};
