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
     * @brief 開始処理 (最初のUpdate直前にCollisionManagerへ登録)
     */
    virtual void Start() override;

    /**
     * @brief 破棄処理 (CollisionManagerから登録解除)
     */
    virtual void OnDestroy() override;

    /**
     * @brief コンポーネント有効化時の通知
     */
    virtual void OnEnable() override;

    /**
     * @brief コンポーネント無効化時の通知
     */
    virtual void OnDisable() override;

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

    /**
     * @brief プロパティの登録を行う
     */
    virtual void OnRegisterProperties() override;

    /**
     * @brief Serialize を実行する。
     */
    virtual nlohmann::json Serialize() override;

    /**
     * @brief Deserialize を実行する。
     */
    virtual void Deserialize(const nlohmann::json& j) override;

    /**
     * @brief デバッグ用の当たり判定枠線（ワイヤーフレーム）を描画する
     */
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
    /**
     * @brief 自身の当たり判定の種類（AABB, Sphere, OBB）を取得する
     * @return コライダー種別 enum
     */
    virtual ColliderType GetColliderType() const = 0;

    /**
     * @brief 空間分割（DynamicBVH）登録用のワールドAABBを取得する
     * @return ワールド空間のバウンディングボックス AABB
     */
    virtual Irufemi::AABB GetBoundingBox() const = 0;

    // --- コールバック機能 ---
    /**
     * @brief 他のコライダーと接触した瞬間に呼ばれるコールバック
     */
    std::function<void(ColliderComponent*)> onCollisionEnter_;

    /**
     * @brief 他のコライダーと接触し続けている間毎フレーム呼ばれるコールバック
     */
    std::function<void(ColliderComponent*)> onCollisionStay_;

    /**
     * @brief 他のコライダーと離れた瞬間に呼ばれるコールバック
     */
    std::function<void(ColliderComponent*)> onCollisionExit_;

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
