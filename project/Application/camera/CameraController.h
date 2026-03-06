#pragma once

#include "Irufemi.h"
#include "camera/Camera.h"

// 前方宣言
class Player;


/**
 * @class CameraController
 * @brief プレイヤーに追従するゲームカメラの振る舞いを制御するクラス
 *
 * プレイヤーを追従し、移動範囲制限やカメラシェイクなどの効果を適用します。
 */
class CameraController {
private:
	// 矩形
	struct Rect {
		// 左端
		float left = 0.0f;
		// 右端
		float right = 1.0f;
		// 下端
		float bottom = 0.0f;
		// 上端
		float top = 1.0f;
	};

	// カメラ
	Camera camera_;

	// 追従対象
	Player* target_ = nullptr;

	// 追従対象と座標の差(オフセット)
	Vector3 targetOffset_ = {0.0f, 0.0f, -15.0f};

	// カメラ移動範囲
	Rect movableArea_ = {11.5f, 87.5f, 6.0f, 100.0f};

	// カメラの目標座標
	Vector3 targetPoint_;

	// 座標補間割合
	static inline const float kInterpolationrate = 0.2f;

	// 速度掛け率
	static inline const float kVelocityBias = 0.3f;

	// 追従対象の各方向へのカメラ移動範囲
	static inline const Rect margin = {-10.0f, 10.0f, -5.0f, 5.0f};

	// カメラシェイク
	float shakeTimer_ = 0.0f;
	float shakeAmplitude_ = 0.0f;

public: // メンバ関数
	/**
	 * @brief 初期化処理
	 */
	void Initialize();

	/**
	 * @brief 更新処理
	 * @param camera 操作対象のカメラ
	 */
	void Update(Camera& camera);

	/**
	 * @brief カメラの位置を追従対象に即座に合わせます
	 */
	void Reset();

	/**
	 * @brief カメラシェイクを開始します
	 * @param duration 持続時間(秒)
	 * @param amplitude 揺れの振幅
	 */
	void StartShake(const float& duration, const float& amplitude);

	// セッター

	void SetTarget(Player* target) { this->target_ = target; }

	void SetMovableArea(const Rect& area) { this->movableArea_ = area; }

	// ゲッター
};
