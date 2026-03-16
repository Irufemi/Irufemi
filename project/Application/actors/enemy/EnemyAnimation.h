#pragma once
#include "core/math/Vector3.h"
#include <array>

class Enemy;
class Player;

class EnemyAnimation {
public:
	void Initialize(Enemy* enemy);
	void Update(Player* player);

	// ビーム本射中かどうか
	bool IsFiring() const { return isFiring_; }

	// 【AI用】ビーム攻撃の一連の流れ（後隙まで）が完了したか
	bool HasFinishedAttack() const { return hasFinishedAttack_; }
	// AI側で次の行動に移る際にリセットする
	void ResetAttackFinished() { hasFinishedAttack_ = false; }

private:
	void UpdateIdle();
	void UpdateAttackBeam(Player* player);
	float NormalizeAngle(float angle);

private:
	Enemy* enemy_ = nullptr;
	float timer_ = 0.0f;
	float attackTimer_ = 0.0f;

	// --- 状態フラグ ---
	bool isLockedOn_ = false;
	bool isFiring_ = false;
	bool hasFinishedAttack_ = false;

	// --- ターゲット固定座標 ---
	Vector3 lockedTargetPos_ = { 0,0,0 };

	// --- 共通パラメータ ---
	float returnSpeed_ = 0.03f;   // 姿勢を戻す時の速度
	float lerpSpeed_ = 0.1f;      // 通常の補間速度

	// --- 待機(Idle)・呼吸パラメータ ---
	float idleRotationSpeed_ = 0.005f;
	float breathSpeed_ = 2.0f;
	float breathHeight_ = 0.25f;
	float bodyWaveHeight_ = 0.15f;
	float phaseOffset_ = 0.6f;

	// --- ビーム攻撃：フェーズ時間設定 (秒) ---
	float chargeTime_ = 2.0f;       // チャージ（追尾）
	float anticipationTime_ = 0.5f; // 溜め（静止）
	float fireTime_ = 1.2f;         // ビーム発射
	float stunTime_ = 0.8f;         // 撃ち終わりの硬直（ここで姿勢を戻す）
	float recoveryTime_ = 2.0f;     // 一呼吸（ガクッと力が抜ける）

	// --- ビーム攻撃：演出パラメータ ---
	float beamRotateSpeed_ = 0.1f;
	float beamThicknessFire_ = 12.0f;
	float beamExpandScale_ = 2.5f;
	float headExtensionY_ = 16.0f;
	float fadeOutStartThreshold_ = 0.85f;

	// --- ビーム攻撃：姿勢・傾き ---
	float fireLeanAngleX_ = 0.25f;   // ビーム中の前傾姿勢（ラジアン）

	// --- ビーム攻撃：シェイク強度 ---
	float shakeBaseSpeed_ = 95.0f;
	float chargeHeadShake_ = 0.35f;  // チャージ中の首の震え
	float chargeBodyShake_ = 0.15f;  // チャージ中の体の震え
	float fireHeadShake_ = 0.75f;    // 発射中の首の震え
	float fireBodyShake_ = 0.35f;    // 発射中の体の震え
	float stunShakeStrength_ = 0.1f; // 硬直中の微細な震え

	// --- 後隙演出 ---
	float exhaustionDepth_ = -2.5f;  // 沈み込む深さ
};