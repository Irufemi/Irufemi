#include"Player.h"

#include "3D/ObjClass.h"
#include "camera/Camera.h"
#include "engine/Input/InputManager.h"
#include "function/Math.h"
#include <cmath>
#include <Xinput.h>
#include <dinput.h>

#include "Sword.h"
#include <algorithm>

static float LerpFloat(float a, float b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return a + (b - a) * t;
}

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
    lastSafePosition_ = pos;

    //必要ならModelの初期角度

   
    seCharge_.Initialize("resources/audio/se/Charge.mp3");
    seSlash_.Initialize("resources/audio/se/Sword.mp3");
   
    chargeVolume_ = 0.0f;
    targetChargeVolume_ = 0.0f;
    seCharge_.SetVolume(0.0f);
}

void Player::Update()
{
    
    constexpr float kFrameDt = 1.0f / 60.0f;
    if (chargeVolume_ != targetChargeVolume_) {
        float alpha = std::clamp(volumeLerpSpeed_ * kFrameDt, 0.0f, 0.5f);
        chargeVolume_ = LerpFloat(chargeVolume_, targetChargeVolume_, alpha);
        seCharge_.SetVolume(chargeVolume_);
      
        if (pendingStopChargeSound_ && chargeVolume_ <= 0.001f) {
            seCharge_.Stop();
            pendingStopChargeSound_ = false;
        }
    }

    // スタン中の場合はタイマーを減らしてスタン終了判定
    if (isStunned_)
    {
        constexpr float kFrameDt = 1.0f / 60.0f;
        stunTimer_ -= kFrameDt;
        if (stunTimer_ <= 0.0f)
        {
            isStunned_ = false;
            stunTimer_ = 0.0f;
        }
        // スタン中は移動や攻撃を行わない。ただし見た目の更新は行う
        transform_.translate += velocity;
        UpdateOBB();
        model_->Debug();
        return;
    }

    lastSafePosition_ = transform_.translate;
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
	// 簡易処理: 衝突時は速度をゼロにして位置を戻す
	velocity = {};
	transform_.translate = lastSafePosition_;

	
}

void Player::Move()
{
	// 移動は攻撃中は不可
	if (isAttacking_)
	{
		velocity = {};
		return;
	}

	// 移動不可: スタン中
	if (isStunned_)
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

   if (isAttacking_ && !attackRangeVisible_ && sword_ && !sword_->IsSlashing()) {
        isAttacking_ = false;
    }


	if (input_->IsKeyDown(VK_SPACE) || input_->IsButtonDown(XINPUT_GAMEPAD_A))
	{
	
		if (!isCharging_) {
			if (isAttacking_ || (sword_ && sword_->IsSlashing())) {
				
				return;
			}

			isCharging_ = true;
			isAttacking_ = true;
			chargeTimer_ = 0.0f;
			attackRangeVisible_ = true;

			
			chargeSoundStarted_ = false;
			targetChargeVolume_ = 0.0f;
			pendingStopChargeSound_ = false;
        }

        
        chargeTimer_ += kFrameDt;
        if (chargeTimer_ > kMaxChargeTime) chargeTimer_ = kMaxChargeTime;

        float ratio = (kMaxChargeTime > 0.0f) ? (chargeTimer_ / kMaxChargeTime) : 1.0f; // 0..1
        attackRangeDistance_ = attackRangeBase_ + (attackRangeMax_ - attackRangeBase_) * ratio;
        float scale = 1.0f + (kAttackRangeMaxScale - 1.0f) * ratio;


        if (!chargeSoundStarted_ && chargeTimer_ >= chargeSoundStartThreshold_) {
            seCharge_.Play(true); 
            chargeSoundStarted_ = true;
        }

        if (chargeSoundStarted_) {
            
            targetChargeVolume_ = 0.2f + 0.4f * ratio; 
        }


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

			
		    if (chargeSoundStarted_) {
		        seCharge_.Stop();
		        chargeVolume_ = 0.0f;
		        targetChargeVolume_ = 0.0f;
		        pendingStopChargeSound_ = false;
		    } else {
		       
		        targetChargeVolume_ = 0.0f;
		        pendingStopChargeSound_ = false;
		    }
		    chargeSoundStarted_ = false;
            
            if (sword_ && attackRangeModel_) {
           
                float chargeRatio = (kMaxChargeTime > 0.0f) ? (chargeTimer_ / kMaxChargeTime) : 0.0f;
                
                const float minDuration = 0.14f;
                const float maxDuration = 0.40f;
             
                float duration = maxDuration + (minDuration - maxDuration) * chargeRatio;

                sword_->StartSlash(attackRangeModel_->GetTransform(), duration);

              
                float vol = 0.5f + 0.5f * chargeRatio;
                seSlash_.SetVolume(vol);
                seSlash_.Play(false);
            }


		}
	}


	if (attackRangeVisible_ && !isCharging_)
	{
		attackRangeTimer_ -= kFrameDt;
		if (attackRangeTimer_ <= 0.0f)
		{
			attackRangeVisible_ = false;

			
			if (!(sword_ && sword_->IsSlashing())) {
				isAttacking_ = false;
			}


			attackRangeDistance_ = attackRangeBase_;
			Transform t = attackRangeModel_->GetTransform();
			t.scale = { 1.0f, 1.0f, 1.0f };
			attackRangeModel_->SetTransform(t);
		}
	}
}
