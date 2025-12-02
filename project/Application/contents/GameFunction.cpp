#include "GameFunction.h"
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
  float closestX = std::max(minX, std::min(circlePos.x, maxX));
  float closestZ = std::max(minZ, std::min(circlePos.z, maxZ));

  // ↑の点とプレイヤーの中心との距離の二乗
  float dx = circlePos.x - closestX;
  float dz = circlePos.z - closestZ;
  float distSq = dx * dx + dz * dz;

  return distSq <= (circleRadius * circleRadius);
}

} // namespace GameFunction