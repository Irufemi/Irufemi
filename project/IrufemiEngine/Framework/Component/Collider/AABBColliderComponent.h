#pragma once
#include "ColliderComponent.h"
#include "Engine/Core/Math/Geometry/AABB.h"
#include "Engine/Core/Math/Vector3.h"
#include <string>

class TransformComponent;

/**
 * @class AABBColliderComponent
 * @brief 軸に平行な箱型の当たり判定コンポーネント
 */
class AABBColliderComponent : public ColliderComponent {
public:
    AABBColliderComponent();
    ~AABBColliderComponent() override;

    void Initialize() override;
    void Update() override;
    void DrawDebug() override;
#ifdef EditorMode
    friend class AABBColliderComponentEditor;
#endif
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    std::string GetComponentName() const override { return "AABBColliderComponent"; }
    ColliderType GetColliderType() const override { return ColliderType::AABB; }

    /// @brief ワールド空間上での現在のAABBを取得
    AABB GetWorldAABB() const;

private:
    TransformComponent* transform_ = nullptr;

    Vector3 localOffset_ = { 0.0f, 0.0f, 0.0f }; //!< 中心からのズレ
    Vector3 localSize_   = { 1.0f, 1.0f, 1.0f }; //!< ボックスの半幅（Extents）
};
