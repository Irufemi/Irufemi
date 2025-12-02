#pragma once
#include "math/Vector3.h"

namespace GameFunction {

/// <summary>
/// 球と球の当たり判定
/// </summary>
/// <param name="posA"></param>
/// <param name="radiusA"></param>
/// <param name="posB"></param>
/// <param name="radiusB"></param>
/// <returns></returns>
bool IsHitSphere(const Vector3 &posA, float radiusA, const Vector3 &posB,
                 float radiusB);

/// <summary>
/// XZ平面での「円(プレイヤー) vs 長方形(壁)」の当たり判定
/// </summary>
/// <param name="circlePos">プレイヤーの中心</param>
/// <param name="circleRadius">プレイヤーの半径</param>
/// <param name="rectCenter">壁の中心</param>
/// <param name="rectWidth">X方向の長さ</param>
/// <param name="rectHeight">Z方向の長さ</param>
/// <returns></returns>
bool IsHitCircleRect(const Vector3 &circlePos, float circleRadius,
                     const Vector3 &rectCenter, float rectWidth,
                     float rectHeight);

void CheckHit_PlayerAndRock();
} // namespace GameFunction