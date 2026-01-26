#include"Player.h"

#include "3D/ObjClass.h"
#include "camera/Camera.h"
#include "engine/Input/InputManager.h"
#include "function/Math.h"
#include <cmath>
#include <Xinput.h>

Player::Player() {}

Player::~Player() {}

void Player::Initialize(Camera* camera, const Vector3& pos, InputManager* input)
{

	CreateObj(camera);

	camera_ = camera;

	input_ = input;

	transform_.translate = pos;

	//必要ならModelの初期角度

}

void Player::Update()
{

	transform_.translate += velocity;

	Move();

	// 進行方向に向ける
	{
		float speed = Math::Length(velocity);
		constexpr float kDeadZone = 1e-4f;
		if (speed > kDeadZone) {
			
			transform_.rotate.z = -std::atan2(velocity.x, velocity.y);
		}
	}

	Attack();

	UpdateOBB();

	model_->Debug();

}

void Player::Draw()
{

	//描画物の位置情報の更新
	model_->SetTransform(transform_);
	model_->Update();

	model_->Draw();

	
	if (attackRangeVisible_) {
		attackRangeModel_->Update();
		attackRangeModel_->Draw();
	}

}

void Player::UpdateOBB()
{
	obb_.center = transform_.translate;
	obb_.size = { width_ / 2.0f, height_ / 2.0f, depth_ / 2.0f };
	Matrix4x4 rotateMatrix = Math::MakeRotateXYZMatrix(transform_.rotate.x, transform_.rotate.y, transform_.rotate.z);
	obb_.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
	obb_.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
	obb_.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };
}

const OBB& Player::GetOBB() const
{
	return obb_;
}

void Player::HandleCollision()
{
	// 簡易処理: 衝突時は速度をゼロにして位置を戻す（ここでは速度のみリセット）
	velocity = {};
}

void Player::Move()
{
	// 移動は攻撃中は不可
	if (isAttacking_) {
		velocity = {};
		return;
	}

	if (input_->IsKeyDown('A')
		|| input_->IsKeyDown('D')
		|| input_->IsKeyDown('W')
		|| input_->IsKeyDown('S')
		|| input_->GetLeftStickX()
		|| input_->GetLeftStickY()
		)
	{
		// 加速度の設定
		Vector3 acceleration = {};

		if (input_->IsKeyDown('A') || input_->GetLeftStickX() < 0.0f)
		{
			acceleration.x = -kAcceleration;
		}

		if (input_->IsKeyDown('D') || input_->GetLeftStickX() > 0.0f)
		{
			acceleration.x = kAcceleration;
		}

		if (input_->IsKeyDown('W') || input_->GetLeftStickY() > 0.0f)
		{
			acceleration.y = kAcceleration;
		}

		if (input_->IsKeyDown('S') || input_->GetLeftStickY() < 0.0f)
		{
			acceleration.y = -kAcceleration;
		}

		velocity = acceleration;
	}
	else
	{
		velocity = {};
	}
}

void Player::CreateObj(Camera* camera)
{
	model_ = std::make_unique<ObjClass>();
	model_->Initialize(camera, "TD_Player.obj");

	attackRangeModel_ = std::make_unique<ObjClass>();
	attackRangeModel_->Initialize(camera, "TD_AttackRange.obj");
}

void Player::Attack()
{

	if (input_ && input_->IsButtonPressed(XINPUT_GAMEPAD_A))
	{
		// 攻撃開始
		isAttacking_ = true;
		velocity = {}; // 攻撃開始時点で動きを止める
	

		attackRangeVisible_ = true;
		attackRangeTimer_ = kAttackRangeDuration;


		Vector3 forward = {};
		float vlen = Math::Length(velocity);
		if (vlen > 1e-4f)
		{
			forward = Math::Normalize(velocity);
		}
		else
		{

			forward.x = std::sin(-transform_.rotate.z);
			forward.y = std::cos(-transform_.rotate.z);
			forward.z = 0.0f;
		}


		Transform t = attackRangeModel_->GetTransform();
		t.translate = transform_.translate + Math::Multiply(attackRangeDistance_, forward);
		t.rotate = transform_.rotate;
		attackRangeModel_->SetTransform(t);
	}


	if (attackRangeVisible_)
	{
		attackRangeTimer_ -= 1.0f / 60.0f;
		if (attackRangeTimer_ <= 0.0f)
		{
			attackRangeVisible_ = false;
			// 攻撃終了
			isAttacking_ = false;
		}
	}
}
