#include "Enemy.h"
#include "function/Math.h"
#include <imgui.h>

void Enemy::Initialize (float x, Vector2 goal) {
	hp_ = 1;
	isAlive_ = true;

	pos_ = { x, -35.0f };
	radius_ = { 25.0f, 25.0f };

	//目標地点までの方向ベクトル(単位ベクトル)を取得
	diff_ = Math::Subtract (goal, pos_);
	normal_ = Math::Normalize (diff_);
	dis_ = 0.0f;
	//velocityを目標地点に向かせる
	velocity_ = Math::Multiply(kEnemySpeed, normal_);
}

bool Enemy::IsCollision (Vector2 pos, float radius) {
	diff_ = Math::Subtract (pos, pos_);
	dis_ = Math::Length (diff_);

	return dis_ <= radius + radius_.x;
}

void Enemy::Update () {
	if (isAlive_) {
		//座標更新
		pos_.x += velocity_.x;
		pos_.y += velocity_.y;
	}
}

void Enemy::Draw () {
	if (isAlive_) {
		//Shape::DrawEllipse (pos_.x, pos_.y, radius_.x, radius_.y, 0.0f, BLACK, kFillModeSolid);
	}
}
