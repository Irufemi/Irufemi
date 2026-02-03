#pragma once

#include <memory>
#include <string>

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
	void Initialize(Camera* camera, const Vector3& pos, const std::string& modelFilename = "TD_Block.obj");
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

	// 壁がどのリングに属しているか設定・取得
	void SetRingIndex(int index) { ringIndex_ = index; }
	int GetRingIndex() const { return ringIndex_; }

#pragma region takamura_追加
	// 修復演出を開始する
	void StartRepairAnimation();

	bool IsRepairing() const { return isRepairing_; }

	float GetAlpha() const { return repairAlpha_; }
#pragma endregion
private:

#pragma region takamura_追加
    // 修復演出用
	bool isRepairing_ = false;       // 修復アニメーション中か
	float repairAnimTimer_ = 0.0f;   // 現在の経過時間
	float repairAnimDuration_ = 1.0f; // 演出にかける秒数（60FPSなら60フレーム分）
	float repairAlpha_ = 0.0f;       // 現在のα値（0.0=透明 → 1.0=不透明）
	Vector3 repairBaseScale_{};      // 演出開始時の最終スケール（目標値）

	float BounceEaseOut(float t);
#pragma endregion

	OBB obb_{};

	// 体力
	int hp_ = 3;
	// 敵と接触しているフレーム数
	int contactFrames_ = 0;
	static inline const int kRequiredContactFrames_ = 60; // 60フレームでダメージ

	float width_ = 8.23f;

	float height_ = 2.0f;

	float depth_ = 2.16f;

	int ringIndex_ = 0; // 壁が属するリングのインデックス

private:
	std::unique_ptr<ObjClass> model_ = nullptr;

	Transform transform_;

	Camera* camera_ = nullptr;
};
