#pragma once
#include "core/math/Vector3.h"
#include <array>

class Enemy;
class Player;

class EnemyAnimation {
public:
	void Initialize(Enemy* enemy);
	void Update(Player* player);

	// ビーム本射中（ダメージ判定有効期間）かどうかを返す
	bool IsFiring() const { return isFiring_; }

private:
	void UpdateIdle();
	void UpdateAttackBeam(Player* player);
	float NormalizeAngle(float angle);

private:
	Enemy* enemy_ = nullptr;
	float timer_ = 0.0f;       // 全体タイマー（サイン波などの時間経過用）
	float attackTimer_ = 0.0f; // 攻撃ステート専用の経過時間

	// --- 状態フラグ ---
	bool isLockedOn_ = false; // ターゲット座標を固定したか
	bool isFiring_ = false;   // ビーム本射中（一番太い状態）か

	// --- 共通パラメータ ---
	float lerpSpeed_ = 0.1f;    // 通常の補間速度
	float returnSpeed_ = 0.05f; // 攻撃終了後、Idleに戻る時の復帰速度

	// --- 待機(Idle)パラメータ ---
	float idleRotationSpeed_ = 0.005f; // Y軸の自転速度
	float idleWaveSpeed_ = 2.0f;       // ふわふわ上下する速さ
	float idleWaveHeight_ = 0.15f;     // 上下揺れの幅
	float idlePhaseOffset_ = 0.8f;     // 各パーツの揺れのズレ

	// --- ビーム攻撃：フェーズ時間（秒） ---
	float chargeTime_ = 2.0f;       // 予兆線で狙っている時間
	float fireTime_ = 5.0f;         // 本射ビームが出ている時間
	float recoveryTime_ = 2.0f;     // 撃ち終わって縮こまっている時間

	// --- ビーム攻撃：挙動パラメータ ---
	float beamRotateSpeed_ = 0.1f;      // チャージ中にプレイヤーを追う回転速度
	float beamThicknessCharge_ = 0.2f;  // 予兆線の太さ
	float beamThicknessFire_ = 12.0f;   // 本射ビームの太さ
	float headExtensionY_ = 16.0f;      // ビームの出る位置の調整Y

	// --- ビーム攻撃：シェイク（振動）パラメータ ---
	float shakeBaseSpeed_ = 70.0f;    // 震えの速さ
	float headShakeStrength_ = 0.3f;  // 頭部の震えの強さ
	float bodyShakeStrength_ = 0.1f;  // ★体部分の震えの強さ（頭より小さめ）

	// --- ビーム攻撃：縮退(Recovery)用ターゲット座標 ---
	float shrinkSpeed_ = 0.12f; // 縮こまる時の移動速度
	// 胴体パーツごとの個別縮退先（パーツ0～2）
	std::array<Vector3, 3> shrinkBodyTargets_ = { {
	   { 0.0f, -1.0f, 0.0f }, // 胴体パーツ0（上段）
	   { 0.0f, -1.5f, 0.0f }, // 胴体パーツ1（中段）
	   { 0.0f, -2.0f, 0.0f }  // 胴体パーツ2（下段：一番沈ませる）
	}};
	// 頭部パーツごとの縮退先
	Vector3 shrinkHeadMidTarget_ = { 0.0f, -4.7f, 1.0f };   // 中央：下かつ前へ
	Vector3 shrinkHeadLeftTarget_ = { 0.5f, -5.0f, 0.5f };  // 左：内側へ寄せる
	Vector3 shrinkHeadRightTarget_ = { -0.5f, -5.0f, 0.5f };// 右：内側へ寄せる

	// ロックオン（発射）した瞬間のターゲット位置を保持
	Vector3 lockedTargetPos_ = { 0.0f, 0.0f, 0.0f };
};