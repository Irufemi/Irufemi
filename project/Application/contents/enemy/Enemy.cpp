#include "Enemy.h"
#include "../wall/Wall.h"
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
	model_->Initialize(camera,"cube.obj");
	transform_.translate = pos;
}

void Enemy::Update(const std::list<Wall*>& walls) {
	if (!alive_) return;

	#pragma region 一番近くのWallに移動する処理

	if (!walls.empty())
	{
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

		if (nearestWall)
		{
			const OBB& wallOBB = nearestWall->GetOBB();
			
			// Enemyの中心からWallのOBBへの最近接点を求める
			Vector3 closestPointOnWallOBB = wallOBB.center; // 仮の初期化
			{
				Vector3 d = transform_.translate - wallOBB.center;
				closestPointOnWallOBB = wallOBB.center;
				const float sizes[] = { wallOBB.size.x, wallOBB.size.y, wallOBB.size.z };
				for (int i = 0; i < 3; ++i) {
					float dist = Math::Dot(d, wallOBB.orientations[i]);
					if (dist > sizes[i]) dist = sizes[i];
					if (dist < -sizes[i]) dist = -sizes[i];
					closestPointOnWallOBB += wallOBB.orientations[i] * dist;
				}
			}


			Vector3 direction = closestPointOnWallOBB - transform_.translate;
			float dirLen = Math::Length(direction);
			
			if (dirLen > 0.0f) {
				direction = Math::Normalize(direction);
				
				float enemyExtent = (width_ > height_) ? width_ : height_;
				enemyExtent = (enemyExtent > depth_) ? enemyExtent : depth_;
				enemyExtent /= 2.0f;

				// 停止距離をEnemyのサイズの半分とする
				float stopDistance = enemyExtent - 0.05f; // 少し重なる設定
				if (stopDistance < 0.01f) stopDistance = 0.01f; // 最小値を確保

				// 移動速度
				speed = 0.1f;                               // 移動速度（適宜調整）

				if (dirLen > stopDistance) {
					
					transform_.translate += direction * speed; // Vector3の演算
				} else {
					// 停止位置は壁に少しめり込むように設定してOBB衝突が発生するようにする
					transform_.translate = closestPointOnWallOBB - direction * stopDistance;
					HandleCollision(); 
					speed = 0.0f;
				}
			}
		}
	}

	#pragma endregion 一番近くのWallに移動する処理

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
