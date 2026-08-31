#pragma once
#include "Framework/Component/Component.h"
#include <functional>

class CollisionManager;

namespace Irufemi {
struct AABB;
}

/**
 * @class ColliderComponent
 * @brief すべての当たり判定コンポーネントの基底クラス
 */
class ColliderComponent : public Component {
public:
    enum class ColliderType { AABB, Sphere, OBB };

    virtual ~ColliderComponent();

    /**
     * @brief Initialize を実行する。
     */
    virtual void Initialize() override {}
    /**
     * @brief Update を実行する。
     */
    virtual void Update() override {}
    /**
     * @brief Draw を実行する。
     */
    virtual void Draw() override {}
    
    virtual void OnRegisterProperties() override;

    /// @brief デバッグ用の当たり判定の枠線を描画する
    virtual void DrawDebug() = 0;

    /**
     * @brief CollisionManager を設定する。
     * @param[in] manager 設定する CollisionManager の値
     */
    static void SetCollisionManager(CollisionManager* manager) {
        collisionManager_ = manager;
    }

protected:
    inline static CollisionManager* collisionManager_ = nullptr;

public:
    /// @brief 自身の当たり判定の種類を返す
    virtual ColliderType GetColliderType() const = 0;

    /// @brief BVH等空間分割用の大まかなAABBを返す
    virtual Irufemi::AABB GetBoundingBox() const = 0;

    // --- コールバック機能 ---
    // 衝突時に呼ばれる関数を登録できる
    std::function<void(ColliderComponent*)> onCollisionEnter_; // 衝突した瞬間に呼ばれる
    std::function<void(ColliderComponent*)> onCollisionStay_;  // 衝突している間呼ばれ続ける
    std::function<void(ColliderComponent*)> onCollisionExit_;  // 離れた瞬間に呼ばれる

    // --- レイヤー設定 ---
    uint32_t layer_ = 1;         // 1 << 0 (Default)
    uint32_t mask_ = 0xFFFFFFFF; // All

    // --- 物理設定 ---
    bool isTrigger_ = false; ///< trueならすり抜ける(判定のみ), falseなら物理的に押し戻す
    bool isStatic_ = false;  ///< trueなら物理的に押し戻されない（環境オブジェクトなど）

    // --- 押し戻し軸の制限 ---
    Irufemi::Vector3 pushbackMask_ = {1.0f, 1.0f, 1.0f}; ///< 1.0 なら押し戻し有効, 0.0 なら無効（Z軸スルーなど）

    // --- BVH (空間分割) 連携 ---
    int32_t bvhNodeId_ = -1; //!< 自身が登録されている Irufemi::DynamicBVH 内のノードインデックス
};
