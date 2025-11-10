#pragma once

#include "math/Vector2.h"

class Enemy{
public:
	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="x">ランダム値</param>
	/// <param name="goal">敵を動かす目標地点</param>
	void Initialize (float x, Vector2 goal);

	/// <summary>
	/// 対象と敵との当たり判定
	/// </summary>
	/// <param name="pos">対象の位置</param>
	/// <param name="radius">対象の半径</param>
	/// <returns>当たってるか</returns>
	bool IsCollision (Vector2 pos, float radius);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update ();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw ();

	bool GetIsAlive () { return isAlive_; }
	void SetIsAlive () { isAlive_ = false; }
	Vector2 GetPosition () { return pos_; }
	Vector2 GetRadius () { return radius_; }
	Vector2 GetDiff () { return diff_; }
	Vector2 GetVelocity() const { return velocity_; } // 追加


private:
	//position
	Vector2  pos_;
	Vector2  radius_;

	//物理演算用の変数
	Vector2 velocity_;

	//敵の情報
	int hp_;
	bool isAlive_;

	//目標地点までの差分ベクトル
	Vector2 diff_;
	//diffを正規化した方向ベクトル
	Vector2 normal_;
	//目標までの距離
	float dis_;

	//敵の速さ
	static inline const float kEnemySpeed = 0.8f;
};

