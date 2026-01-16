#pragma once

#include <memory>

#include "math/Transform.h"
#include "3D/ObjClass.h"

// 前方宣言
class Camera;

/// <summary>
/// 天球
/// </summary>
class Skydome {
private: // メンバ変数
	// ワールド変換データ
	Transform worldTransform_;

	// モデル
	std::unique_ptr<ObjClass> model_ = nullptr;

	// カメラ
	Camera* camera_ = nullptr;

public: // メンバ関数
	    /// <summary>
	    /// 初期化
	    /// </summary>
	void Initialize(Camera* camera);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();
};
