#pragma once
#include "core/math/Vector3.h"
#include <array>

class Enemy;
class Player;

class EnemyAnimation {
public:
    void Initialize(Enemy* enemy);
    void Update(Player* player);

    // ビーム本射中（ダメージ判定を出して良い期間）かどうかを外部に教える
    bool IsFiring() const { return isFiring_; }

private:
    void UpdateIdle();
    void UpdateAttackBeam(Player* player);
    float NormalizeAngle(float angle);

private:
    Enemy* enemy_ = nullptr;
    float timer_ = 0.0f;       // 全体タイマー
    float attackTimer_ = 0.0f; // 攻撃中専用のタイマー

    // --- 状態フラグ ---
    bool isLockedOn_ = false; // ターゲット座標を固定したかどうか
    bool isFiring_ = false;   // ビームが出ているかどうか

    // --- 共通パラメータ ---
    float lerpSpeed_ = 0.1f;    // 通常の滑らかさ
    float returnSpeed_ = 0.05f; // 攻撃からIdleへ戻る時の滑らかさ

    // --- 待機(Idle)時のパラメータ ---
    float idleRotationSpeed_ = 0.005f; // くるくる回る速さ
    float idleWaveSpeed_ = 2.0f;       // 上下揺れの速さ
    float idleWaveHeight_ = 0.15f;     // 上下揺れの幅

    // --- ビーム攻撃：時間設定（秒） ---
    float chargeTime_ = 2.0f;   // 狙いを定めている時間
    float fireTime_ = 5.0f;     // ビームぶっ放し時間
    float recoveryTime_ = 2.0f; // 撃ち終わりの賢者タイム（硬直）

    // --- ビーム攻撃：挙動 ---
    float beamRotateSpeed_ = 0.1f;      // 追尾の機敏さ
    float beamThicknessCharge_ = 0.2f;  // 予兆の細さ
    float beamThicknessFire_ = 12.0f;   // ビームの太さ（さらに太く！）

    // --- ビーム攻撃：振動(シェイク) ---
    float shakeStrength_ = 0.3f;   // ビーム中の震えの強さ
    float shakeBaseSpeed_ = 70.0f; // ビーム中の震えの速さ

    // --- ビーム攻撃：後隙(Recovery)の縮こまりターゲット座標 ---
    float shrinkSpeed_ = 0.12f; // 縮こまる時の速さ（Lerp係数）

    // ★胴体パーツごとの縮退先を配列に（パーツ0～2）
    // 真ん中に寄せつつ、高さをバラけさせて「潰れた」感じを出します
    std::array<Vector3, 3> shrinkBodyTargets_ = { {
        { 0.0f, -1.0f, 0.0f }, // 胴体パーツ0（上段）
        { 0.0f, -1.5f, 0.0f },// 胴体パーツ1（中段）
        { 0.0f, -2.0f, 0.0f }  // 胴体パーツ2（下段：一番沈ませる）
    } };

    // 各頭部の縮退先
    Vector3 shrinkHeadMidTarget_ = { 0.0f, -3.7f, 1.0f };   // 中央：下かつ前へ
    Vector3 shrinkHeadLeftTarget_ = { 1.0f, -4.0f, 0.5f };  // 左：内側へ寄せる
    Vector3 shrinkHeadRightTarget_ = { -1.0f, -4.0f, 0.5f };// 右：内側へ寄せる

    // 固定したターゲット位置
    Vector3 lockedTargetPos_ = { 0.0f, 0.0f, 0.0f };
};