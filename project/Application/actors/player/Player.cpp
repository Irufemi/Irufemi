#include"Player.h"

#include "3D/ObjClass.h"
#include "camera/Camera.h"
#include "engine/Input/InputManager.h"
#include "function/Math.h"
#include <cmath>
#include <Xinput.h>
#include <dinput.h>

#include "Sword.h"

Player::Player() {}

Player::~Player() {}

void Player::Initialize(Camera* camera, const Vector3& pos, InputManager* input)
{
	if (!model_ && !attackRangeModel_) {
		CreateObj(camera);
	}

	// create sword
	sword_ = std::make_unique<Sword>();
	sword_->Initialize(camera, pos);

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
		if (speed > kDeadZone)
		{

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


	if (attackRangeVisible_)
	{
		//attackRangeModel_->Update();
		//attackRangeModel_->Draw();

		
		if (sword_)
		{
			Transform t = attackRangeModel_->GetTransform();
			sword_->SetTransform(t);
			sword_->Update();
			sword_->Draw();
		}
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
	if (isAttacking_)
	{
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
	constexpr float kFrameDt = 1.0f / 60.0f;
	constexpr float kAttackRangeMaxScale = 3.0f;


	if (input_->IsKeyDown(VK_SPACE) || input_->IsButtonDown(XINPUT_GAMEPAD_A))
	{
		if (!isCharging_)
		{

			isCharging_ = true;
			isAttacking_ = true;
			chargeTimer_ = 0.0f;
			attackRangeVisible_ = true;
		}


		chargeTimer_ += kFrameDt;
		if (chargeTimer_ > kMaxChargeTime) chargeTimer_ = kMaxChargeTime;

		float ratio = (kMaxChargeTime > 0.0f) ? (chargeTimer_ / kMaxChargeTime) : 1.0f; // 0..1
		attackRangeDistance_ = attackRangeBase_ + (attackRangeMax_ - attackRangeBase_) * ratio;
		float scale = 1.0f + (kAttackRangeMaxScale - 1.0f) * ratio;


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

		// モデルの尖端(ローカル)を基準にスケール時の位置調整を行う
		float tipLocal = attackRangeModelTipOffset_;
		float scaledTipLocal = tipLocal * scale * attackRangeModelTipDirection_;


		Transform t = attackRangeModel_->GetTransform();
		t.translate = transform_.translate + Math::Multiply(attackRangeTipAnchorDistance_ - scaledTipLocal, forward);
		t.rotate = transform_.rotate;
		t.scale = { scale, scale, scale };
		attackRangeModel_->SetTransform(t);

		return;
	}


	if ((input_ && input_->IsButtonReleased(XINPUT_GAMEPAD_A)) || (input_ && input_->IsKeyReleased(VK_SPACE)))
	{
		if (isCharging_)
		{

			isCharging_ = false;
			attackRangeVisible_ = true;
			attackRangeTimer_ = kAttackRangeDuration;


			
			if (sword_ && attackRangeModel_) {
				sword_->StartSlash(attackRangeModel_->GetTransform());
			}


		}
	}


	if (attackRangeVisible_ && !isCharging_)
	{
		attackRangeTimer_ -= kFrameDt;
		if (attackRangeTimer_ <= 0.0f)
		{
			attackRangeVisible_ = false;
			isAttacking_ = false;


			attackRangeDistance_ = attackRangeBase_;
			Transform t = attackRangeModel_->GetTransform();
			t.scale = { 1.0f, 1.0f, 1.0f };
			attackRangeModel_->SetTransform(t);
		}
	}
}
