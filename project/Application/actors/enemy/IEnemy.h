#pragma once

#include "Irufemi.h"

#include "Core/Type/LRDirection.h"
#include "contents/mapChipField/MapChipField.h"

#include <memory>

// 前方宣言
class Player;
class ObjClass;

/**
 * @class IEnemy
 * @brief 敵キャラクターの基底クラス（インターフェース）
 * @details 全ての敵クラスが継承すべき共通の機能とインターフェースを定義します。
 *          移動、衝突判定、描画などの基本的な振る舞いを持ち、
 *          具体的なAIや攻撃方法は派生クラスで実装されます。
 */
class IEnemy
{
private: // 内部型
    /**
     * @struct CollisionMapInfo
     * @brief マップとの衝突情報を格納する構造体
     */
    struct CollisionMapInfo {
        bool isContactCeiling = false;
        bool isContactGround = false;
        bool isContactWall = false;
        int  wallDir = 0;
        Vector3 amountMove{};
    };

public: // メンバ関数
    /**
     * @brief デストラクタ
     */
    virtual ~IEnemy() = default;

    /**
     * @brief 敵の初期化処理
     * @param position 初期座標
     */
    virtual void Initialize(const Vector3& position);
    /**
     * @brief 毎フレームの更新処理
     * @details 派生クラスで具体的な振る舞いを実装する必要があります。
     */
    virtual void Update() = 0;
    /**
     * @brief 描画処理
     * @details 派生クラスでモデルの描画を実装する必要があります。
     */
    virtual void Draw() = 0;

    /**
     * @brief プレイヤーとの衝突時に呼ばれる処理
     * @param player 衝突したプレイヤーのポインタ
     */
    virtual void OnCollision(Player* player);

    /**
     * @brief マップチップフィールドを設定します
     * @param mapChipField マップチップフィールドのポインタ
     */
    void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

public: // アクセサ
    /**
     * @brief AABB(軸並行境界ボックス)を取得します
     * @return AABB 敵の当たり判定
     */
    AABB GetAABB() const;
    /**
     * @brief 生存フラグを取得します
     * @return bool trueなら死亡、falseなら生存
     */
    bool IsDead() const { return isDead_; }
    /**
     * @brief 現在の向きを取得します
     * @return LRDirection 左右の向き
     */
    LRDirection GetLRDirection() const { return lrDirection_; }
    /**
     * @brief プレイヤーに与えるダメージ量を取得します
     * @return int ダメージ量
     */
    int GetDamage() const { return damage_; }
    /**
     * @brief ワールド座標を取得します
     * @return Vector3 ワールド座標
     */
    Vector3 GetWorldPosition() const;

protected: // 派生クラス向けアクセサ
    const Transform& GetTransform() const { return transform_; }
    Transform& GetTransform() { return transform_; }
    const Vector3& GetVelocity() const { return velocity_; }
    void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }
    LRDirection GetDirection() const { return lrDirection_; }
    void SetDirection(const LRDirection& direction) { lrDirection_ = direction; }
    bool IsOnGround() const { return onGround_; }
    bool IsTouchingWall() const { return isTouchingWall_; }
    void SetIsDead(const bool& isDead) { isDead_ = isDead; }
    std::unique_ptr<ObjClass>& GetModel() { return model_; }
    void SetWidth(const float& width) { width_ = width; }
    void SetHeight(const float& height) { height_ = height; }
    void SetDamage(const int& damage) { damage_ = damage; }

protected: // 内部処理
    /**
     * @brief 移動と衝突判定を含む基本的な更新処理
     * @details 地面や壁を検知して自動で方向転換する基本的なAIを提供します。
     */
    void BehaviorMoveUpdate();
    /**
     * @brief 重力を適用します
     */
    void ApplyGravity();
    /**
     * @brief マップとの衝突検知を行います
     * @param[out] info 衝突結果を格納する構造体
     */
    void CollisionDetection(CollisionMapInfo& info);
    /**
     * @brief 衝突解決後の移動量を座標に適用します
     * @param info 衝突情報
     */
    void MoveAccordingly(const CollisionMapInfo& info);
    /**
     * @brief 地面との接触時の処理
     * @param info 衝突情報
     */
    void ContactGround(const CollisionMapInfo& info);
    /**
     * @brief 壁との接触時の処理
     * @param info 衝突情報
     */
    void ContactWall(const CollisionMapInfo& info);
    /**
     * @brief 指定座標がマップの固いブロック内にあるか判定します
     * @param p 判定するワールド座標
     * @param[out] outIdx 対応するマップチップのインデックス
     * @param[out] outRect 対応するマップチップの矩形
     * @return bool 固いブロック内ならtrue
     */
    bool IsSolidAt(const Vector3& p, MapChipField::IndexSet* outIdx, MapChipField::Rect* outRect) const;
    /**
     * @brief 垂直方向の移動量を衝突解決します
     * @param base 現在の基準座標
     * @param dy 垂直方向の移動量
     * @param[out] info 衝突情報を更新
     * @return float 解決後の垂直移動量
     */
    float ResolveVerticalFrom(const Vector3& base, float dy, CollisionMapInfo& info) const;
    /**
     * @brief 水平方向の移動量を衝突解決します
     * @param base 現在の基準座標
     * @param dx 水平方向の移動量
     * @param[out] info 衝突情報を更新
     * @return float 解決後の水平移動量
     */
    float ResolveHorizontalFrom(const Vector3& base, float dx, CollisionMapInfo& info) const;
    /**
     * @brief ワールド行列を更新します
     */
    void UpdateMatrix();

protected: // 定数
    static inline const float kgravityAcceleration = 0.010f;
    static inline const float kLimitFallSpeed = 0.36f;
    static inline const float kMBlank = 0.01f;
    static inline const float kDefaultMoveSpeed = 0.05f;

private: // メンバ変数
    // トランスフォーム
    Transform transform_;
    // ワールド行列
    Matrix4x4 worldMatrix_;
    // モデル
    std::unique_ptr<ObjClass> model_ = nullptr;
    // 幅
    float width_ = 1.0f;
    // 高さ
    float height_ = 1.0f;
    // 生存フラグ
    bool isDead_ = false;
    // 向き
    LRDirection lrDirection_ = LRDirection::kLeft;
    // ダメージ
    int damage_ = 10;
    // マップチップフィールド
    MapChipField* mapChipField_ = nullptr;
    // 速度
    Vector3 velocity_{};
    // 接地フラグ
    bool onGround_ = false;
    // 壁接触フラグ
    bool isTouchingWall_ = false;
};

