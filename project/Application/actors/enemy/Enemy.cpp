#include "Enemy.h"
#include "contents/wall/Wall.h"
#include "function/Random.h"
#include "../healer/HealerActor.h"
#include <limits>
#include <cfloat>

#include "camera/Camera.h"

#include "3D/ObjClass.h"

#include "function/Math.h"
#include "function/Collision.h"

Enemy::Enemy() {}

Enemy::~Enemy() {}

void Enemy::Initialize(Camera* camera, Vector3 pos) {
    camera_ = camera;
    model_ = std::make_unique<ObjClass>();
    model_->Initialize(camera, "cube.obj");
    transform_.translate = pos;
	preferHealer_ = false;
	preferHealerTimer_ = 0;
}

void Enemy::Update(const std::list<Wall*>& walls, const std::list<HealerActor*>& healers) {
	if (!alive_) {
		// カウントダウンでリスポーン判定
		if (respawnCounter_ > 0) {
			--respawnCounter_;
		} else {
			// リスポーン
			alive_ = true;
			// ランダム位置に再配置（適宜範囲は調整）
			float x = Random::GeneratorFloat(-10.0f, 10.0f);
			float y = Random::GeneratorFloat(-10.0f, 10.0f);
			transform_.translate = Vector3{ x, y, 0.0f };
			// 初期速度をリセット
			speed = 0.0f;
		}
		// UpdateAABB と transform 更新はリスポーン後も行う
		UpdateOBB();
		return;
	}

	if (preferHealerTimer_ <= 0) {
		// roll once and cache result for kPreferHealerFrames
		float prob = Random::GeneratorFloat(0.0f, 1.0f);
		preferHealer_ = (prob <= 0.5f);
		preferHealerTimer_ = kPreferHealerFrames;
	} else {
		--preferHealerTimer_;
	}

	// まずは修復中のHealerActorの位置を優先して狙う（ただし確率で切り替える）
	HealerActor const* targetHealer = nullptr;
	float bestHealerDist = FLT_MAX;
	for (HealerActor const* ha : healers) {
		if (!ha) continue;
		if (!ha->IsAssigned()) continue; // 修復中のもののみ
		float d = Math::Length(ha->GetPosition() - transform_.translate);
		if (d < bestHealerDist) { bestHealerDist = d; targetHealer = ha; }
	}

	Vector3 targetPos{ 0,0,0 };
	bool hasTarget = false;
	if (targetHealer && preferHealer_) {
		hasTarget = true;
		targetPos = targetHealer->GetPosition();
	} else {
		// 修復中のHealerを選ばなかった、または存在しなかった場合は従来通り最寄りのWallを狙う
		Wall* nearestWall = nullptr;

		float nearestDistance = FLT_MAX;

		// 一番近いWallを探す
		for (Wall* wall : walls)
		{
			if (!wall)
			{
				continue;
			}
			Vector3 toWall = wall->GetPosition() - transform_.translate;
			float distance = Math::Length(toWall);
			if (distance < nearestDistance) {
				nearestDistance = distance;
				nearestWall = wall;
			}
		}

		if (nearestWall) {
			hasTarget = true;
			targetPos = nearestWall->GetPosition();
		}
	}

	if (hasTarget) {
		Vector3 direction = targetPos - transform_.translate;
		float dirLen = Math::Length(direction);

		if (dirLen > 0.0f) {
			direction = Math::Normalize(direction);

			// 移動速度
			speed = 0.1f;                               // 移動速度（適宜調整）

			transform_.translate += direction * speed; // Vector3の演算
		}
	}

    UpdateOBB();

}

void Enemy::Draw() {

    // 描画物の更新
    model_->SetTransform(transform_);
    model_->Update();

    // 描画物の描画
    if (alive_ && model_) model_->Draw();
}

void Enemy::UpdateOBB()
{
    obb_.center = transform_.translate;
    obb_.size = { width_ / 2.0f, height_ / 2.0f, depth_ / 2.0f };
    Matrix4x4 rotateMatrix = Math::MakeRotateXYZMatrix(transform_.rotate.x, transform_.rotate.y, transform_.rotate.z);
    obb_.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
    obb_.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
    obb_.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };
}

const OBB& Enemy::GetOBB() const { return obb_; }

void Enemy::HandleCollision() { speed = 0.0f; }

void Enemy::Kill() { alive_ = false; respawnCounter_ = kRespawnFrames; }