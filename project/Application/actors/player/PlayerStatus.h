#pragma once

#include "Irufemi.h"
#include "Engine/Core/Math/Geometry/OBB.h"

// 前方宣言
class Enemy;
class IrufemiEngine;

/**
 * @struct PlayerCollider
 * @brief プレイヤー自身の当たり判定（攻撃を受ける側）データ
 */
struct PlayerCollider {
    Vector3 center; // 判定の中心座標
    float radius;   // 判定の半径
    OBB obb;        // OBBの当たり判定データ
};

class PlayerStatus {
public:
    PlayerStatus() = default;
    ~PlayerStatus() = default;

    // 初期化
    void Initialize();

    // タイマーなどの更新
    void Update();

    // 吹き飛ばし中の敵の座標更新
    void UpdateKnockback();

    // ダメージ処理
    void ApplyDamage(int damage, bool isCharging, IrufemiEngine* engine);

    // 敵を吹き飛ばす処理の開始
    void HitAndKnockback(Enemy* enemy, const Vector3& playerTranslate);

    // 当たり判定の取得（プレイヤーの座標と回転、揺れを考慮して生成）
    PlayerCollider GetCollider(const Vector3& playerTranslate, const Vector3& playerRotate, const Vector3& missileVibration) const;

    // ゲッター
    int GetHp() const { return hp_; }
    int GetMaxHp() const { return kMaxHp; }
    bool IsDead() const { return isDead_; }
    int GetInvincibleTimer() const { return invincibleTimer_; }

    // ★追加: セッター（回避時に無敵時間を付与するため）
    void SetInvincibleTimer(int time) { invincibleTimer_ = time; }

private:
    // --- ステータス・やられ判定用 ---
    static constexpr int kMaxHp = 100;           // 最大体力
    static constexpr float kColliderRadius = 1.0f; // プレイヤーのやられ判定半径

    // --- ノックバック用定数 ---
    static constexpr int kKnockbackDuration = 20;
    static constexpr float kKnockbackPower = 2.0f;
    static constexpr float kKnockbackFriction = 0.85f;
    static constexpr float kKnockbackMinDistance = 0.001f;

    int hp_ = kMaxHp;
    bool isDead_ = false;
    int invincibleTimer_ = 0;

    // 敵を吹き飛ばす（ノックバック）用データ
    Enemy* knockbackTarget_ = nullptr;
    Vector3 knockbackVelocity_ = { 0.0f, 0.0f, 0.0f };
    int knockbackTimer_ = 0;
};