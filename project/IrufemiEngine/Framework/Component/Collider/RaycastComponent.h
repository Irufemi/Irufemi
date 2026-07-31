#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Manager/CollisionManager.h"
#include <functional>

class TransformComponent;

/**
 * @class RaycastComponent
 * @brief 自身の位置から指定した方向へレイを飛ばし、オブジェクトを検知するセンサーコンポーネント
 */
class RaycastComponent : public Component {
public:
    RaycastComponent() = default;
    ~RaycastComponent() = default;

    /**
     * @brief Initialize を実行する。
     */
    void Initialize() override;
    /**
     * @brief Update を実行する。
     */
    void Update() override;
    /**
     * @brief DrawDebug を実行する。
     */
    void DrawDebug(); // ComponentにはDrawDebugがないためoverrideを外す
    
    /**
     * @brief CollisionManager を設定する。
     * @param[in] manager 設定する CollisionManager の値
     */
    static void SetCollisionManager(CollisionManager* manager) { collisionManager_ = manager; }
    
protected:
    inline static CollisionManager* collisionManager_ = nullptr;

#ifdef EditorMode
    friend class RaycastComponentEditor;
#endif
    /**
     * @brief Serialize を実行する。
     */
    nlohmann::json Serialize() override;
    /**
     * @brief Deserialize を実行する。
     */
    void Deserialize(const nlohmann::json& j) override;

    /// @brief 現在レイが何かに当たっているかを取得する
    bool IsHit() const { return hitInfo_.isHit; }
    
    /// @brief 当たったオブジェクトの情報を取得する
    const RaycastHit& GetHitInfo() const { return hitInfo_; }

    // 設定
    Irufemi::Vector3 localOffset_ = { 0.0f, 0.0f, 0.0f };
    Irufemi::Vector3 localDirection_ = { 0.0f, 0.0f, 1.0f }; // ローカルZ軸方向
    float maxDistance_ = 100.0f;
    uint32_t mask_ = 0xFFFFFFFF; // 全てのレイヤーと判定

    bool showDebugLine_ = true;

    // 当たった時に呼ばれるコールバック
    std::function<void(const RaycastHit&)> onHit_;

private:
    RaycastHit hitInfo_;
    Irufemi::Ray currentRay_;
};
