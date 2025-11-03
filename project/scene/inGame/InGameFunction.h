#pragma once

#include "math/Vector2.h"
#include "math/shape/LinePrimitive.h"
#include "CollisionResult.h"

//反射ベクトルを求める関数
Vector2 Reflect (const Vector2& input, Vector2& normal);

//当たり判定の関数群
CollisionResult isCollision (const Vector2& pos, const Vector2& radius, const Segment2D& segment);