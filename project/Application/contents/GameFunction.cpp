#include "GameFunction.h"
#include "actor/enemy/Enemy.h"
#include "actor/player/Player.h"
#include "actor/rock/RockManager.h"
#include <algorithm>

namespace GameFunction {
bool IsHitSphere(const Vector3 &posA, float radiusA, const Vector3 &posB,
                 float radiusB) {

  Vector3 diff = posA - posB;

  float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

  float r = radiusA + radiusB;

  return distSq <= r * r;
}

bool IsHitCircleRect(const Vector3 &circlePos, float circleRadius,
                     const Vector3 &rectCenter, float rectWidth,
                     float rectHeight) {

  // 壁のサイズの半分
  float halfW = rectWidth * 0.5f;
  float halfH = rectHeight * 0.5f;

  // 壁の境界(四点)
  float minX = rectCenter.x - halfW;
  float maxX = rectCenter.x + halfW;
  float minZ = rectCenter.z - halfH;
  float maxZ = rectCenter.z + halfH;

  // 壁の中で一番プレイヤーに近い点
  float closestX = (std::max)(minX, (std::min)(circlePos.x, maxX));
  float closestZ = (std::max)(minZ, (std::min)(circlePos.z, maxZ));

  // ↑の点とプレイヤーの中心との距離の二乗
  float dx = circlePos.x - closestX;
  float dz = circlePos.z - closestZ;
  float distSq = dx * dx + dz * dz;

  return distSq <= (circleRadius * circleRadius);
}

// 岩とプレイヤー
void CheckHit_PlayerAndRock(Player &player, std::vector<Rock> &rocks) {
  const Vector3 &pPos = player.GetPosition();
  const float pRadius = player.GetRadius();

  for (auto &rock : rocks) {
    if (!rock.isAlive_) {
      continue;
    }

    if (IsHitSphere(pPos, pRadius, rock.position_, rock.radius_)) {
      // プレイヤーが岩を拾う
      player.AddRock(1);

      // 岩を消す（isAlive_ = false）
      rock.Kill();
    }
  }
}

bool isHitCircle(const Vector3 &posA, float radiusA, const Vector3 &posB,
                 float radiusB) {
  // XZ 平面での距離
  float dx = posA.x - posB.x;
  float dz = posA.z - posB.z;

  // 距離の2乗
  float distSq = dx * dx + dz * dz;

  // 半径の和
  float r = radiusA + radiusB;

  // 衝突（=中心距離 <= 半径の和）
  return distSq <= (r * r);
}

float DistanceXZ(const Vector3& a, const Vector3& b) {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}

} // namespace GameFunction
