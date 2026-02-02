#include "HealerActor.h"


#include "function/Math.h"

#include "Camera/Camera.h"
#include <cmath>

#include "contents/wall/Wall.h"

HealerActor::HealerActor() {}

HealerActor::~HealerActor() {}

void HealerActor::Initialize(Camera* camera, const Vector3& pos) {

	camera_ = camera;

	transform_.translate = pos;
	transform_.scale = { 1.0f, 1.0f, 1.0f };
	targetPosition_ = pos;

	model_ = std::make_unique<ObjClass>();
	model_->Initialize(camera_,"TD_Healer.obj");

}

void HealerActor::Update() {
	if (!alive_)
	{
		return;
	}
	UpdateOBB();
}

void HealerActor::Draw() {
	if (!alive_)
	{
		return;
	}
	model_->SetTransform(transform_);
	model_->Update();
	model_->Draw();
}

Vector3 HealerActor::GetPosition() const {
	return transform_.translate;
}

void HealerActor::MoveTowards(const Vector3& target, float speed, const std::list<Wall*>& walls) {
    Vector3& pos = transform_.translate;
    Vector3 toTarget = { target.x - pos.x, target.y - pos.y, target.z - pos.z };
    float distToTarget = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

    if (distToTarget < 1e-4f) return;

    // 本来進みたい方向
    Vector3 desiredDir = { toTarget.x / distToTarget, toTarget.y / distToTarget, toTarget.z / distToTarget };

    // --- 障害物（壁）の検知 ---
    Vector3 avoidanceDir = { 0, 0, 0 };
    bool needsAvoidance = false;
    float lookAhead = 2.0f; // 2.0ユニット先まで壁をチェック

    for (Wall* wall : walls) {
        if (!wall) continue;

        Vector3 wallPos = wall->GetPosition();
        Vector3 toWall = { wallPos.x - pos.x, wallPos.y - pos.y, wallPos.z - pos.z };

        // 1. 進行方向（desiredDir）上にあるか判定
        float projection = toWall.x * desiredDir.x + toWall.y * desiredDir.y + toWall.z * desiredDir.z;

        // 壁が前方、かつ一定距離以内にいる場合
        if (projection > 0 && projection < lookAhead) {
            // 2. 進行ルートから壁の距離を計算（最近点との距離）
            Vector3 closestPointOnPath = {
                pos.x + desiredDir.x * projection,
                pos.y + desiredDir.y * projection,
                pos.z + desiredDir.z * projection
            };

            float distFromPathSq =
                std::powf(wallPos.x - closestPointOnPath.x, 2) +
                std::powf(wallPos.y - closestPointOnPath.y, 2) +
                std::powf(wallPos.z - closestPointOnPath.z, 2);

            // 壁の半径（約2.0〜2.5）より近ければ「ぶつかる」と判断
            float wallRadius = 2.2f;
            if (distFromPathSq < (wallRadius * wallRadius)) {
                // 3. 回避ベクトルを計算
                // 進行方向に対して垂直に逃げる力を加える
                Vector3 lateralForce = {
                    closestPointOnPath.x - wallPos.x,
                    closestPointOnPath.y - wallPos.y,
                    0 // 2D的な移動ならZは0
                };

                // 正規化して回避方向を合成
                float latLen = std::sqrt(lateralForce.x * lateralForce.x + lateralForce.y * lateralForce.y);
                if (latLen > 1e-4f) {
                    avoidanceDir.x += (lateralForce.x / latLen) * (lookAhead - projection);
                    avoidanceDir.y += (lateralForce.y / latLen) * (lookAhead - projection);
                    needsAvoidance = true;
                }
            }
        }
    }

    // --- 移動方向の決定 ---
    Vector3 finalDir = desiredDir;
    if (needsAvoidance) {
        // 本来の目的方向と回避方向を混ぜる
        finalDir.x += avoidanceDir.x * 1.5f; // 回避の重みを少し強くする
        finalDir.y += avoidanceDir.y * 1.5f;

        // ベクトルを正規化し直す
        float finalLen = std::sqrt(finalDir.x * finalDir.x + finalDir.y * finalDir.y + finalDir.z * finalDir.z);
        finalDir.x /= finalLen;
        finalDir.y /= finalLen;
        finalDir.z /= finalLen;
    }

    // 移動実行
    pos.x += finalDir.x * speed;
    pos.y += finalDir.y * speed;
    pos.z += finalDir.z * speed;
}

void HealerActor::RefreshTransform() {

}

void HealerActor::SetAssigned(bool assigned) { assigned_ = assigned; }

bool HealerActor::IsAssigned() const { return assigned_; }

void HealerActor::SetAlive(bool alive) { alive_ = alive; }

bool HealerActor::IsAlive() const { return alive_; }

void HealerActor::SetTargetPosition(const Vector3& pos) { targetPosition_ = pos; }

const Vector3& HealerActor::GetTargetPosition() const { return targetPosition_; }


void HealerActor::UpdateOBB() {
	obb_.center = transform_.translate;
	obb_.size = { width_ / 2.0f, height_ / 2.0f, depth_ / 2.0f };
	Matrix4x4 rotateMatrix = Math::MakeRotateXYZMatrix(transform_.rotate.x, transform_.rotate.y, transform_.rotate.z);
	obb_.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
	obb_.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
	obb_.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };
}

const OBB& HealerActor::GetOBB() const {
	return obb_;
}

void HealerActor::HandleCollision() {
	alive_ = false;
}
