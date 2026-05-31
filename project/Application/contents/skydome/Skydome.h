#pragma once

#include <memory>

#include "Irufemi.h"

// 前方宣言
class Camera;

/**
 * @class Skydome
 * @brief 背景となる天球モデルを管理するクラス
 */
class Skydome {
private: // メンバ変数
	// ワールド変換データ
	Transform worldTransform_;

	// モデル
	std::unique_ptr<ObjClass> model_ = nullptr;

	// カメラ

	// --- 回転・ゆらぎ演出用パラメータ ---
	float timer_ = 0.0f;               ///< 内部タイマー
	float baseRotY_ = 0.0f;            ///< 累積のY軸回転量
	
	Vector3 baseTilt_ = { 0.2f, 0.0f, 0.1f };  ///< 地軸のような初期の傾き（ラジアン）
	float rotationSpeedY_ = 0.0005f;           ///< ゆったりとした基本のY軸回転速度
	float wobbleSpeed_ = 0.1f;                 ///< ゆらぎの波の速さ
	float wobbleAmplitude_ = 0.015f;           ///< ゆらぎの振幅


public: // メンバ関数
	/**
	 * @brief 初期化処理
	 * @param camera カメラのポインタ
	 */
	void Initialize();

	/**
	 * @brief 更新処理
	 */
	void Update();

	/**
	 * @brief 描画処理
	 */
	void Draw();

	/**
	 * @brief 天球の色（明るさ）を設定
	 */
	void SetColor(const Vector4& color);

	/**
	 * @brief 天球をY軸回転させる
	 */
	void AddRotateY(float rotY);
};
