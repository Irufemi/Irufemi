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
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    std::string GetComponentName() const override { return "AABBColliderComponent"; }
    ColliderType GetColliderType() const override { return ColliderType::AABB; }
    Irufemi::AABB GetBoundingBox() const override { return GetWorldAABB(); }

    /// @brief ワールド空間上での現在のAABBを取得
    Irufemi::AABB GetWorldAABB() const;

    /**
     * @brief ボックスの中心からのズレ（ローカル座標）を設定します
     * @param offset 設定するオフセット値
     */
    void SetLocalOffset(const Irufemi::Vector3& offset) { localOffset_ = offset; }

    /**
     * @brief ボックスの中心からのズレ（ローカル座標）を取得します
     * @return const Irufemi::Vector3& オフセット値
     */
    const Irufemi::Vector3& GetLocalOffset() const { return localOffset_; }

    /**
     * @brief ボックスの半幅（Extents）を設定します
     * @param size 設定する半幅
     */
    void SetLocalSize(const Irufemi::Vector3& size) { localSize_ = size; }

    /**
     * @brief ボックスの半幅（Extents）を取得します
     * @return const Irufemi::Vector3& 半幅
     */
    const Irufemi::Vector3& GetLocalSize() const { return localSize_; }

private:
    TransformComponent* transform_ = nullptr;

    Irufemi::Vector3 localOffset_ = { 0.0f, 0.0f, 0.0f }; //!< 中心からのズレ
    Irufemi::Vector3 localSize_   = { 1.0f, 1.0f, 1.0f }; //!< ボックスの半幅（Extents）
};
