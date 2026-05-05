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

};
