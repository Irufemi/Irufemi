#include"Player.h"

#include "3D/ObjClass.h"
#include "camera/Camera.h"
#include "engine/Input/InputManager.h"
#include "function/Math.h"
#include <cmath>

Player::Player() {}

Player::~Player() {}

void Player::Initialize(Camera* camera, const Vector3& pos, InputManager* input)
{

	model_ = std::make_unique<ObjClass>();
	model_->Initialize(camera, "TD_Player.obj");

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

	UpdateOBB();

	model_->Debug();

}

void Player::Draw()
{

	//描画物の位置情報の更新
	model_->SetTransform(transform_);
	model_->Update();

	model_->Draw();

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
