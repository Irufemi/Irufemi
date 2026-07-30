#pragma once
#include "Component.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/System/ComponentPool.h"

class TransformComponent : public Component {
public:
    void Initialize() override {}
    // Update() は一括更新 (UpdateAll) に移行したため完全に削除し、GameObjectのUpdateループから外す
    
    /**
     * @brief 自身の行列を計算する（Dirtyフラグベースの遅延評価付き）
     * @param force 親が変更された等の理由による強制更新フラグ
     */
    void ComputeMatrix(bool force = false);

    /**
     * @brief プール上の全Transformを一括更新する（DOD）
     */
    static void UpdateAll();

    bool CanUpdateInEditMode() const override { return true; }

    // --- Getters ---
    const Irufemi::Vector3& GetPosition() const { return position_; }
    const Irufemi::Vector3& GetRotation() const { return rotation_; }
    const Irufemi::Vector3& GetScale() const { return scale_; }

    const Irufemi::Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
    const Irufemi::Matrix4x4& GetLocalMatrix() const { return localMatrix_; }

    // ワールド情報の遅延抽出 Getter
    const Irufemi::Vector3& GetWorldPosition() const;
    const Irufemi::Vector3& GetWorldRotation() const;
    const Irufemi::Vector3& GetWorldScale() const;

    // ワールド方向ベクトルの抽出 (正規化済み)
    Irufemi::Vector3 GetWorldRight() const;
    Irufemi::Vector3 GetWorldUp() const;
    Irufemi::Vector3 GetWorldForward() const;

    // --- Setters ---
    void SetPosition(const Irufemi::Vector3& position);
    void SetRotation(const Irufemi::Vector3& rotation);
    void SetScale(const Irufemi::Vector3& scale);

    std::string GetComponentName() const override { return "TransformComponent"; }
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

#ifdef EditorMode

#endif

private:
    // --- Local Irufemi::Transform Data ---
    Irufemi::Vector3 position_ = { 0.0f, 0.0f, 0.0f };
    Irufemi::Vector3 rotation_ = { 0.0f, 0.0f, 0.0f }; // Euler angles in radians
    Irufemi::Vector3 scale_ = { 1.0f, 1.0f, 1.0f };

    // --- World Irufemi::Transform Data (Lazy Evaluated) ---
    mutable Irufemi::Vector3 worldPosition_ = { 0.0f, 0.0f, 0.0f };
    mutable Irufemi::Vector3 worldRotation_ = { 0.0f, 0.0f, 0.0f };
    mutable Irufemi::Vector3 worldScale_ = { 1.0f, 1.0f, 1.0f };

    // --- Matrices ---
    Irufemi::Matrix4x4 localMatrix_ = Irufemi::Math::MakeIdentity4x4();
    Irufemi::Matrix4x4 worldMatrix_ = Irufemi::Math::MakeIdentity4x4();

    // --- Flags ---
    bool isLocalDirty_ = true;
    bool isWorldDirty_ = true;
    mutable bool isWorldTransformExtracted_ = false;

    // UpdateAll用のフレームキャッシュ
    uint32_t lastUpdateFrame_ = 0;
    static inline uint32_t currentFrame_ = 1;
};

// ComponentPool 対応を宣言
template<> struct IsPooledComponent<TransformComponent> : std::true_type {};
