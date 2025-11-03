#include "InGameFunction.h"

#include <cmath>
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include "function/Math.h"

Vector2 Reflect(const Vector2& input, Vector2& normal) {
    Vector2 reflect;

    normal = Math::Normalize(normal);

    float dot = Math::Dot(input, normal);

    reflect = input - normal * (2.0f * dot);

    return reflect;
}

CollisionResult isCollision(const Vector2& pos, const Vector2& radius, const Segment2D& segment) {
	CollisionResult result;
	//ぷれいやーと地面の最近接点を求める
	result.closest = Math::ClosestPoint(pos, segment);

	//プレイヤーから最近接点までのベクトルを出す
	Vector2 diff = Math::Subtract(pos, result.closest);
	float distance = Math::Length(diff);

	//距離よりプレイヤーの位置+半径が小さいか
	if (distance < radius.x) {
		result.isColliding = true;
	} else {
		result.isColliding = false;
	}

	//線分の方向ベクトルから法線を作る
	Vector2 ab = Math::Subtract(segment.end, segment.origin);
	//求めたベクトルを回転させて法線にする
	result.normal = Math::Normalize(Vector2{ -ab.y, ab.x });
	//内積をチェックして向きを調整
	if (Math::Dot(result.normal, diff) < 0.0f) {
		result.normal = Math::Multiply(-1.0f, result.normal);
	}

	//押し戻す量を求める(半径-距離)
	result.penetration = radius.x - distance;

	return result;
}


