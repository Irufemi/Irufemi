#pragma once
#include "../Component.h"
#include <functional>

/**
 * @class ColliderComponent
 * @brief すべての当たり判定コンポーネントの基底クラス
 */
class ColliderComponent : public Component {
public:
    enum class ColliderType { AABB, Sphere, OBB };

    // レイヤーの定義（ビットマスク）
    enum CollisionLayer : uint32_t {
        Default = 1 << 0,
        Player  = 1 << 1,
        Enemy   = 1 << 2,
        Bullet  = 1 << 3,
        Wall    = 1 << 4,
        All     = 0xFFFFFFFF
    };

    virtual ~ColliderComponent() = default;

    virtual void Initialize() override {}
    virtual void Update() override {}
    virtual void Draw() override {}
    
    /// @brief デバッグ用の当たり判定の枠線を描画する
    virtual void DrawDebug() = 0;

    /// @brief 自身の当たり判定の種類を返す
    virtual ColliderType GetColliderType() const = 0;

    // --- コールバック機能 ---
    // 衝突時に呼ばれる関数を登録できる
    std::function<void(ColliderComponent*)> onCollisionEnter_;

    // --- レイヤー設定 ---
    uint32_t layer_ = CollisionLayer::Default;
    uint32_t mask_  = CollisionLayer::All;
};
