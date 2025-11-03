#include "Player.h"
#include <cmath>
#include "engine/Input/InputManager.h"
#include "engine/Input/GamePad.h"
#include "camera/Camera.h"
#include <dinput.h>
#include <imgui.h>
#include "InGameFunction.h"

#include "ScreenSpace.h"


void Player::Initialize (InputManager* inputManager, Camera* camera) {

    camera_ = camera;
    inputManager_ = inputManager;
    // GamePad のデッドゾーンを設定（左スティックのノイズを無視）
    if (inputManager_ && inputManager_->GetGamePad()) {
        inputManager_->GetGamePad()->SetLeftDeadZone(0.25f);
    }

	pos_ = { 100.0f, 400.0f };
	radius_ = { 40.0f, 40.0f };
	velocity_ = { -1.0f, 0.0f };

	cannonPos_ = { pos_.x, pos_.y - radius_.y };
	cannonRadius_ = { 18.0f, 30.0f };
	cannonOffset_ = { 0.0f, -30.0f };
	angle_ = 0.0f;
	rad_ = 0.0f;
	sinf_ = 0.0f;
	cosf_ = 0.0f;
	reflect_ = { 0.0f, 0.0f };
	wallTouch_ = false;
	bulletNum_ = 10;
	isStan_ = false;
	stanTime_ = 60;
	disToCore_ = 0.0f;

	for (auto& b : bullet) {
		b.Initialize(pos_, sinf_, cosf_,camera_);
	}

    sphere_ = std::make_unique<SphereClass>();
    sphere_->SetInfo(Sphere{ Vector3{pos_.x,pos_.y,0.0f},radius_.x });
    sphere_->Initialize(camera,"resources/whiteTexture.png");
	sphere_->SetColor(normalColor_);

    // 砲塔（Cylinder）を生成
    cylinder_ = std::make_unique<CylinderClass>();
    {
        // 初期位置をZ=0平面のワールドに変換して半径/高さもワールド化
        Vector3 wcCannon = ScreenToWorldOnZ(camera, cannonPos_, 0.0f);
        float  rWorld    = ScreenRadiusToWorld(camera, cannonPos_, cannonRadius_.x, 0.0f);
        float  hWorld    = ScreenRadiusToWorld(camera, cannonPos_, cannonRadius_.y, 0.0f) * 2.0f; // 矩形の高さ相当
        cylinder_->SetInfo(Cylinder{ wcCannon, rWorld, hWorld });
    }
    cylinder_->Initialize(camera, "resources/whiteTexture.png");
	cylinder_->SetColor(normalColor_);

	se_playerAction_ = std::make_unique<Se>();
	se_playerAction_->Initialize("resources/se/SE_PlayerAction.mp3");

	se_bullet = std::make_unique<Se>();
	se_bullet->Initialize("resources/se/SE_Bullet.mp3");

	se_playertoutch = std::make_unique<Se>();
	se_playertoutch->Initialize("resources/se/by_chance.mp3");

	
}

void Player::Jump () {
	if (!inputManager_) return;
	GamePad* gp = inputManager_->GetGamePad();
	bool gpA = gp ? gp->IsButtonPressed(XINPUT_GAMEPAD_A) : false;

	if ((inputManager_->IsKeyPressedDIK(DIK_SPACE) || gpA) && bulletNum_ > 0) {
		if (pos_.x <= 250.0f) {
			velocity_.x = 4.0f;
			velocity_.y = 6.0f;
		}
		else if (pos_.x >= 250.0f) {
			velocity_.x = -4.0f;
			velocity_.y = 6.0f;
		}
	}
}

void Player::Rotate () {
	if (!inputManager_) return;

	constexpr float kGamepadDeadzone = 0.25f;
	GamePad* gp = inputManager_->GetGamePad();

	bool leftDown  = inputManager_->IsKeyDownDIK(DIK_A);
	bool rightDown = inputManager_->IsKeyDownDIK(DIK_D);

	// 左スティック（アナログ）を優先してチェック
	if (gp) {
		float lx = gp->GetLeftStickX();
		if (lx < -kGamepadDeadzone) leftDown = true;
		if (lx >  kGamepadDeadzone) rightDown = true;
		// 十字キーも許可
		if (gp->DPadLeft())  leftDown = true;
		if (gp->DPadRight()) rightDown = true;
	}

	if (leftDown)  angle_ -= 5.0f;
	if (rightDown) angle_ += 5.0f;

	// --- ラジアン変換 ---
	rad_ = angle_ * (3.14159265f / 180.0f);

	// --- 回転を適用（オフセットを回転） ---
	Vector2 rotatedOffset;
	rotatedOffset.x = cannonOffset_.x * cosf(rad_) - cannonOffset_.y * sinf(rad_);
	rotatedOffset.y = cannonOffset_.x * sinf(rad_) + cannonOffset_.y * cosf(rad_);

	// --- プレイヤー位置を中心に戻す ---
	cannonPos_.x = pos_.x + rotatedOffset.x;
	cannonPos_.y = pos_.y + rotatedOffset.y;
}

void Player::Fire () {
	if (!inputManager_) return;
	GamePad* gp = inputManager_->GetGamePad();
	bool gpA = gp ? gp->IsButtonPressed(XINPUT_GAMEPAD_A) : false;

	if ((inputManager_->IsKeyPressedDIK(DIK_SPACE) || gpA) && bulletNum_ > 0) {
		se_playerAction_->Play();
		for (auto& b : bullet) {
			if (!b.GetIsActive()) {
				b.Initialize(pos_, sinf(rad_), cosf(rad_), camera_);
				b.SetIsActive(true);
				bulletNum_--;
				break;
			}
		}
	}
}

void Player::SpeedCalculation () {
	if (pos_.x - radius_.x <= 0.0f || pos_.x + radius_.x >= 500.0f) {
		wallTouch_ = true;
		se_playertoutch->Play();
		velocity_.x *= -1.0f;
	}
	if (pos_.y - radius_.y <= 0.0f) {
		velocity_.y = 0.0f;
	}

	velocity_.y += kGravity * deltaTime;
}

void Player::Input () {
	if (!isStan_) {
		Jump();
		Fire();
	}
}

void Player::Stan() {
	if (isStan_ == false)return;

	if (stanTime_ > 0) {
		stanTime_--;
	}

	if (stanTime_ <= 0) {
		isStan_ = false;

		sphere_->SetColor(normalColor_);
		cylinder_->SetColor(normalColor_);
	}
}

void Player::disCalculation(Vector2 pos) {
	Vector2 vec = Math::Subtract(pos, pos_);
	disToCore_ = { Math::Length(vec) };
}

void Player::Update () {

	Stan();

	//bulletNumの上限
	if (bulletNum_ >= kMaxBullet) {
		bulletNum_ = kMaxBullet;
	}

	//座標更新
	pos_.x += velocity_.x;
	pos_.y -= velocity_.y;

	//壁へのめり込み予防
	if (pos_.x - radius_.x < 0.0f) {
		pos_.x = 0.0f + radius_.x;
	} else if (pos_.x + radius_.x > 500.0f) {
		pos_.x = 500.0f - radius_.x;
	}

	//大砲
	Rotate();

	//弾
	for (auto& b : bullet) {
		b.Update();

		if (b.IsRecovered()) {
			b.SetIsReturn(true);
			b.SetIsActive(false); 
			bulletNum_ += 1;
			se_bullet->Play();
		}

		if (b.GetIsReturn()) {
			b.SetPosition(b.Return(pos_, b.easeInExpo(b.moveT(0.5f))));
			if (b.GetT() > 1.0f) {
				b.SetIsReturn(false);
				b.Initialize(Vector2{1000.0f, 1000.0f}, 0.0f, 0.0f,camera_);
				b.SetIsActive(false);
			}
		}
#if defined(_DEBUG) || defined(DEVELOPMENT)
		ImGui::Text("isActive: %d", b.GetIsActive());
#endif
	}
}

void Player::DrawSet() {
    Vector3 wc = ScreenToWorldOnZ(camera_, pos_, 0.0f);
    float wr = ScreenRadiusToWorld(camera_, pos_, radius_.x, 0.0f);
    sphere_->SetInfo(Sphere{ wc, wr });
    // 2D角度（rad_）をZ回転へ
    sphere_->SetRotate(Vector3{ 0.0f, 0.0f, -rad_ });
    sphere_->Update("PlayerSphere");

    // 砲塔のローカル矩形を更新（弾の発射座標算出などで使うなら残す）
    Vector2 local[4] = {
        {-cannonRadius_.x, -cannonRadius_.y},
        { cannonRadius_.x, -cannonRadius_.y},
        {-cannonRadius_.x,  cannonRadius_.y},
        { cannonRadius_.x,  cannonRadius_.y},
    };
    for (int i = 0; i < 4; i++) {
        float x = local[i].x;
        float y = local[i].y;
        local[i].x = x * cosf(rad_) - y * sinf(rad_);
        local[i].y = x * sinf(rad_) + y * cosf(rad_);
        local[i].x += cannonPos_.x;
        local[i].y += cannonPos_.y;
    }

    // 砲塔（Cylinder）を毎フレーム反映
    {
        Vector3 wcCannon = ScreenToWorldOnZ(camera_, cannonPos_, 0.0f);
        float  rWorld    = ScreenRadiusToWorld(camera_, cannonPos_, cannonRadius_.x, 0.0f);
        float  hWorld    = ScreenRadiusToWorld(camera_, cannonPos_, cannonRadius_.y, 0.0f) * 2.0f;

        cylinder_->SetInfo(Cylinder{ wcCannon, rWorld, hWorld });
        // 2D回転をZ回転として適用（CylinderはY軸が高さ）
        cylinder_->SetRotate(Vector3{ 0.0f, 0.0f, -rad_ });
        cylinder_->Update("Cannon");
    }
}

void Player::Draw () {
    // 砲塔 → プレイヤーの順でもOK（深度有効なら順不同）
    cylinder_->Draw();
    sphere_->Draw();
}

void Player::BulletDraw() {

	// 弾
	for (auto& b : bullet) {
		b.Draw();
	}
}

void Player::CollectBullet(int num) {
	int collectedCount = 0;

	for (size_t i = 0; i < bullet.size(); ++i) {
		//現在の弾
		auto& b = bullet[i];

		if (b.GetIsActive()) {
			b.Collect();
			collectedCount++;
		}

		if (collectedCount >= num) {
			break;
		}
	}
}
