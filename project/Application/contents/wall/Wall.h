#pragma once

#include <memory>

#include "math/Vector3.h"
#include "math/Transform.h"
#include "math/shape/OBB.h"

// 前方宣言
class Camera;
class ObjClass;

// 血管(壁)
class Wall {
public:
	Wall();
	~Wall();
	void Initialize(Camera* camera, const Vector3& pos);
	void Update();
	void Draw();

	void UpdateOBB();

	// OBB の取得
	const OBB& GetOBB() const;

	// Transformの取得
	const Transform& GetTransform() const;

	// 位置の取得
	const Vector3& GetPosition() const;

	// 回転を設定（z軸回転など）
	void SetRotation(const Vector3& rot);

	// スケールを設定
	void SetScale(const Vector3& scale);

	// 回転の取得
	const Vector3& GetRotation() const;

	// ダメージ蓄積（敵に触れたフレームをカウントし、一定フレームでHPを減らす）
	// 戻り値: true の場合、HP が 0 以下となり破壊される
	bool AccumulateContactFrame();

	// 接触フレームを徐々に減らす（即時リセットではなくゆっくりデクリメント）
	void DecayContactFrames();

	int GetHP() const { return hp_; }
	void ResetContactFrames() { contactFrames_ = 0; }

	float GetWidth() { return width_; }
	float GetHeight() { return height_; }
	float GetDepth() { return depth_; }

	// サイズをVector3として取得
	Vector3 GetSize() const;

private:
	OBB obb_{};

	// 体力
	int hp_ = 3;
	// 敵と接触しているフレーム数
	int contactFrames_ = 0;
	static inline const int kRequiredContactFrames_ = 60; // 60フレームでダメージ

	float width_ = 8.23f;

	float height_ = 2.0f;

	float depth_ = 2.16f;

private:
	std::unique_ptr<ObjClass> model_ = nullptr;

	Transform transform_;

	Camera* camera_ = nullptr;
};
