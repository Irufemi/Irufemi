#pragma once

#include "Irufemi.h"
#include "contents/mapChipField/MapChipField.h"

class Player;
class InputManager;

/**
 * @class PlayerPhysics
 * @brief プレイヤーの物理挙動と移動ロジックを専門に扱うクラス
 * @details プレイヤーの移動入力、重力、壁との衝突、ジャンプ、ダッシュなど、
 *          物理的な振る舞いに関する計算と状態管理を担当します。
 *          Playerクラスから利用されるコンポーネントとして設計されています。
 */
class PlayerPhysics {
public:
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

    /**
     * @brief 初期化処理
     * @param player 親となるPlayerオブジェクト
     * @param transform プレイヤーのTransform
     * @param mapChipField マップデータ
     * @param inputManager 入力マネージャー
     */
    void Initialize(Player* player, Transform* transform, MapChipField* mapChipField, InputManager* inputManager);
    /**
     * @brief 毎フレームの更新処理 (現在は未使用)
     */
    void Update();
    /**
     * @brief 入力に基づいた移動処理
     * @details 左右移動、ジャンプ、壁ジャンプなどの入力を処理し、速度を更新します。
     */
    void MoveInput();
    /**
     * @brief 重力を適用します
     */
    void ApplyGravity();
    /**
     * @brief 移動と衝突判定を含む更新処理
     * @details 旋回、マップとの衝突検知、座標の更新を行います。
     */
    void BehaviorMoveUpdate();

    // ゲッター
    const Vector3& GetVelocity() const { return velocity_; }
    bool IsOnGround() const { return onGround_; }
    bool IsTouchingWall() const { return isTouchingWall_; }
    int GetLastWallDir() const { return lastWallDir_; }
    int GetWallCoyoteCounter() const { return wallCoyoteCounter_; }
    int GetAirJumpsLeft() const { return airJumpsLeft_; }
    bool IsDashUsed() const { return dashUsed_; }

    // セッター
    void SetVelocity(const Vector3& vel) { velocity_ = vel; }
    void SetAirJumpsLeft(int count) { airJumpsLeft_ = count; }
    void SetDashUsed(bool used) { dashUsed_ = used; }
    void ResetHorizontalLock() { horizontalControlLockTimer_ = 0.0f; }


private:
    /**
     * @brief プレイヤーの向きに応じた旋回処理
     */
    void TurningControl();
    /**
     * @brief マップとの衝突検知
     * @param[out] info 衝突結果を格納する構造体
     */
    void CollisionDetection(CollisionMapInfo& info);
    /**
     * @brief 衝突解決後の移動量を座標に適用
     * @param info 衝突情報
     */
    void MoveAccordingly(const CollisionMapInfo& info);
    /**
     * @brief 天井との接触処理
     * @param info 衝突情報
     */
    void ContactCeiling(const CollisionMapInfo& info);
    /**
     * @brief 地面との接触処理
     * @param info 衝突情報
     */
    void ContactGround(const CollisionMapInfo& info);
    /**
     * @brief 壁との接触処理
     * @param info 衝突情報
     */
    void ContactWall(const CollisionMapInfo& info);
    /**
     * @brief 指定座標がマップの固いブロック内にあるか判定
     * @param p 判定するワールド座標
     * @param[out] outIdx 対応するマップチップのインデックス
     * @param[out] outRect 対応するマップチップの矩形
     * @return bool 固いブロック内ならtrue
     */
    bool IsSolidAt(const Vector3& p, MapChipField::IndexSet* outIdx, MapChipField::Rect* outRect) const;
    /**
     * @brief 垂直方向の移動量を衝突解決
     * @param base 現在の基準座標
     * @param dy 垂直方向の移動量
     * @param[out] info 衝突情報を更新
     * @return float 解決後の垂直移動量
     */
    float ResolveVerticalFrom(const Vector3& base, float dy, CollisionMapInfo& info) const;
    /**
     * @brief 水平方向の移動量を衝突解決
     * @param base 現在の基準座標
     * @param dx 水平方向の移動量
     * @param[out] info 衝突情報を更新
     * @return float 解決後の水平移動量
     */
    float ResolveHorizontalFrom(const Vector3& base, float dx, CollisionMapInfo& info) const;

private:
    Player* player_ = nullptr;
    Transform* transform_ = nullptr;
    MapChipField* mapChipField_ = nullptr;
    InputManager* inputManager_ = nullptr;

    Vector3 velocity_{};
    bool onGround_ = true;
    int coyoteCounter_ = 0;
    int jumpBufferCounter_ = 0;
    int airJumpsLeft_ = 0;
    bool jumpHeldPrev_ = false;
    bool isTouchingWall_ = false;
    int lastWallDir_ = 0;
    int wallCoyoteCounter_ = 0;
    float horizontalControlLockTimer_ = 0.0f;
    bool dashUsed_ = false;

    // 旋回用
    float turnFirstRotationY_ = 0.0f;
    float turnTimer_ = 0.0f;
};