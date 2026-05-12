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
};
