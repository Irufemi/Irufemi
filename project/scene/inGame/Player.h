#pragma once
#include "Bullet.h"
#include <array>

#include "math/Vector2.h"
#include "3D/SphereClass.h"
#include "3D/CylinderClass.h"
#include "audio/Se.h"
#include <memory>

class Camera;

class InputManager;

class Player{
public:
	void Initialize (InputManager* inputManager,Camera * camera);

	//固有の処理
	void Jump();
	void Rotate();
	void Fire();
	void SpeedCalculation();
	void Input();
	void Stan();
	void disCalculation(Vector2 pos);
	void DrawSet();
	void Update();
	void Draw();
	void BulletDraw();

	//プレイヤー
	Vector2 GetPositon() { return pos_; }
	void SetPosition(Vector2 pos) { pos_ = pos; }
	Vector2 GetRadius() { return radius_; }
	Vector2 GetVelocity() { return velocity_; }
	void SetVelocity(Vector2 velocity) { velocity_ = velocity; }
	Vector2 GetReflect() { return reflect_; }
	void SetReflect(Vector2 velocity) { reflect_ = velocity; }
	bool GetWallTouch() { return wallTouch_; }
	void SetWallTouch() { wallTouch_ = false; }
	int GetBulletNum() { return bulletNum_; }
	void SetBulletNum() { bulletNum_++; }
	bool GetIsStan() { return isStan_; }
	void SetIsStan() {
		stanTime_ = 60;
		isStan_ = true;
		sphere_->SetColor(stanColor_);
		cylinder_->SetColor(stanColor_);
	}
	float GetDisToCore() { return disToCore_; }

	//弾
	std::array<Bullet, 10>& GetBullet() { return bullet; }
	void CollectBullet(int num);

private:
	// 描画用生成物
	std::unique_ptr<SphereClass> sphere_ = nullptr;
	std::unique_ptr<CylinderClass> cylinder_ = nullptr;

	Vector4 normalColor_ = { 0.4f,0.4f,0.7f,1.0f };
	Vector4 stanColor_ = { 0.7f,0.4f,0.4f,1.0f };

	std::unique_ptr<Se> se_playerAction_ = nullptr;

	std::unique_ptr<Se> se_bullet = nullptr;

	std::unique_ptr<Se> se_playertoutch = nullptr;

private:
	//position
	Vector2  pos_;
	Vector2  radius_;

	//物理演算用の変数
	Vector2 velocity_;

	//砲台の変数
	Vector2 cannonPos_;
	Vector2 cannonRadius_;
	//回転用
	Vector2 cannonOffset_;
	float angle_;
	float rad_;
	float sinf_;
	float cosf_;
	Vector2 newPos_;
	//反射ベクトル
	Vector2 reflect_;
	//壁に触ったか
	bool wallTouch_;
	//残弾数
	int bulletNum_;
	//スタン
	bool isStan_;
	int stanTime_;
	//コアとの距離を常に取っておく
	float disToCore_;

	//弾
	std::array<Bullet, 10> bullet;

	static inline const float kGravity = -8.5f;
	static inline const float kMaxFallSpeed = 20.0f;
	static inline const float deltaTime = 1.0f / 60.0f;
	//めり込み防止用の定数
	static inline const float kPos = 3.0f;
	//bulletNumの上限
	static inline const int kMaxBullet = 10;

	// 外部参照

	//キーボード
	InputManager* inputManager_ = nullptr;
	Camera* camera_ = nullptr;
};

