#include "Collision.h"

#include "Math.h"
#include "math/shape/AABB.h"
#include "math/shape/LinePrimitive.h"
#include "math/shape/Plane.h"
#include "math/shape/Sphere.h"
#include "math/shape/Triangle.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Collision {

    // 球と球の衝突判定
    bool IsCollision(const Vector3& s1_center, const float& s1_radius, const Vector3& s2_center, const float& s2_radius) {

        // 2つの球の中心点間の距離を求める
        float distance = Math::Length(Math::Subtract(s2_center, s1_center));
        // 半径の合計よりも短ければ衝突
        if (distance <= s1_radius + s2_radius) {
            // 当たった処理を諸々
            return true;
        }

        return false;
    }

    // 球と球の衝突判定
    bool IsCollision(const Sphere& s1, const Sphere& s2) {

        // 2つの球の中心点間の距離を求める
        float distance = Math::Length(Math::Subtract(s2.center, s1.center));
        // 半径の合計よりも短ければ衝突
        if (distance <= s1.radius + s2.radius) {
            // 当たった処理を諸々
            return true;
        }

        return false;
    }

    // 平面と球の衝突判定
    bool IsCollision(const Sphere& sphere, const Plane& plane) {
        float k = std::fabs(Math::Dot(plane.normal, sphere.center) - plane.distance);
        if (k <= sphere.radius) {
            return true;
        }
        return false;
    }

    // 線分と平面の衝突判定
    bool IsCollision(const Segment& segment, const Plane& plane) {

        // まずは垂直判定を行うために、法線と線の内積を求める
        float dot = Math::Dot(plane.normal, segment.diff);

        // 垂直 = 平行であるので、衝突しているはずがない
        if (dot == 0.0f) {
            return false;
        }

        // tを求める
        float t = (plane.distance - Math::Dot(segment.origin, plane.normal)) / dot;

        // tの値と線の種類によって衝突しているかを判断する

        // segmentのため範囲は0.0f ~ 1.0f

        if (0.0f <= t && t <= 1.0f) {
            return true;
        } else {
            return false;
        }
    }

    // 半直線と平面の衝突判定
    bool IsCollision(const Ray& ray, const Plane& plane) {

        // まずは垂直判定を行うために、法線と線の内積を求める
        float dot = Math::Dot(plane.normal, ray.diff);

        // 垂直 = 平行であるので、衝突しているはずがない
        if (dot == 0.0f) {
            return false;
        }

        // tを求める
        float t = (plane.distance - Math::Dot(ray.origin, plane.normal)) / dot;

        // tの値と線の種類によって衝突しているかを判断する

        // rayのため範囲は0.0f ~

        if (0.0f <= t) {
            return true;
        } else {
            return false;
        }
    }

    // 直線と平面の衝突判定
    bool IsCollision(const Line& line, const Plane& plane) {

        // まずは垂直判定を行うために、法線と線の内積を求める
        float dot = Math::Dot(plane.normal, line.diff);

        // 垂直 = 平行であるので、衝突しているはずがない
        if (dot == 0.0f) {
            return false;
        }

        // lineのため範囲は無制限

        return true;
    }

    // 三角形と線分の衝突判定
    bool IsCollision(const Triangle& triangle, const Segment& segment) {

        // 各辺を結んだベクトルと、頂点と衝突点pを結んだベクトルのクロス積を求める
        Vector3 normal = Math::Cross(Math::Subtract(triangle.vertices_[1], triangle.vertices_[0]), Math::Subtract(triangle.vertices_[2], triangle.vertices_[0]));

        // 平面と線分の内積（垂直＝平行チェック）
        float dot = Math::Dot(normal, segment.diff);
        if (dot == 0.0f) {
            return false; // 平行なので交差しない
        }

        // 平面と線分の交点を求める
        float t = Math::Dot(normal, Math::Subtract(triangle.vertices_[0], segment.origin)) / dot;
        // t が [0,1] にないなら線分上に交点がない
        if (t < 0.0f || t > 1.0f) {
            return false;
        }

        // 交点を求める
        Vector3 p = Math::Add(segment.origin, Math::Multiply(t, segment.diff));

        // 各辺を結んだベクトルと、頂点と衝突点pを結んだベクトルのクロス積を取る
        Vector3 v01 = Math::Subtract(triangle.vertices_[1], triangle.vertices_[0]);
        Vector3 v1p = Math::Subtract(p, triangle.vertices_[1]);
        Vector3 cross01 = Math::Cross(v01, v1p);
        Vector3 v12 = Math::Subtract(triangle.vertices_[2], triangle.vertices_[1]);
        Vector3 v2p = Math::Subtract(p, triangle.vertices_[2]);
        Vector3 cross12 = Math::Cross(v12, v2p);
        Vector3 v20 = Math::Subtract(triangle.vertices_[0], triangle.vertices_[2]);
        Vector3 v0p = Math::Subtract(p, triangle.vertices_[0]);
        Vector3 cross20 = Math::Cross(v20, v0p);
        // すべての小三角形のクロス積と法線が同じ方向を向いていたら衝突
        if (Math::Dot(cross01, normal) >= 0.0f && Math::Dot(cross12, normal) >= 0.0f && Math::Dot(cross20, normal) >= 0.0f) {
            // 衝突
            return true;
        }
        return false;
    }

    // AABBとAABBの衝突判定
    bool IsCollision(const AABB& a, const AABB& b) {

        if ((a.min.x <= b.max.x && a.max.x >= b.min.x) && // x軸
            (a.min.y <= b.max.y && a.max.y >= b.min.y) && // y軸
            (a.min.z <= b.max.z && a.max.z >= b.min.z)    // z軸
            ) {
            return true;
        }

        return false;
    }

    // AABBと球の衝突判定
    bool IsCollision(const AABB& aabb, const Sphere& sphere) {

        // 最近接点を求める
        Vector3 closestPoint{ std::clamp(sphere.center.x, aabb.min.x, aabb.max.x), std::clamp(sphere.center.y, aabb.min.y, aabb.max.y), std::clamp(sphere.center.z, aabb.min.z, aabb.max.z) };
        // 最近接点と球の中心との距離を求める
        float distance = Math::Length(Math::Subtract(closestPoint, sphere.center));
        // 距離が半径よりも小さければ衝突
        if (distance <= sphere.radius) {
            // 衝突
            return true;
        }

        return false;
    }

    // AABBと線分の衝突判定
    bool IsCollision(const AABB& aabb, const Segment& segment) {

        float tMin = 0.0f;
        float tMax = 1.0f;

        // x軸
        if (segment.diff.x != 0.0f) {
            float tx1 = (aabb.min.x - segment.origin.x) / segment.diff.x;
            float tx2 = (aabb.max.x - segment.origin.x) / segment.diff.x;
            float tNearX = std::min(tx1, tx2);
            float tFarX = std::max(tx1, tx2);
            tMin = std::max(tMin, tNearX);
            tMax = std::min(tMax, tFarX);
        } else {
            if (segment.origin.x < aabb.min.x || segment.origin.x > aabb.max.x) {
                return false;
            }
        }

        // y軸
        if (segment.diff.y != 0.0f) {
            float ty1 = (aabb.min.y - segment.origin.y) / segment.diff.y;
            float ty2 = (aabb.max.y - segment.origin.y) / segment.diff.y;
            float tNearY = std::min(ty1, ty2);
            float tFarY = std::max(ty1, ty2);
            tMin = std::max(tMin, tNearY);
            tMax = std::min(tMax, tFarY);
        } else {
            if (segment.origin.y < aabb.min.y || segment.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (segment.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - segment.origin.z) / segment.diff.z;
            float tz2 = (aabb.max.z - segment.origin.z) / segment.diff.z;
            float tNearZ = std::min(tz1, tz2);
            float tFarZ = std::max(tz1, tz2);
            tMin = std::max(tMin, tNearZ);
            tMax = std::min(tMax, tFarZ);
        } else {
            if (segment.origin.z < aabb.min.z || segment.origin.z > aabb.max.z) {
                return false;
            }
        }

        // 衝突
        if (tMin <= tMax) {
            return true;
        }

        return false;
    }

    // AABBと半直線の衝突判定
    bool IsCollision(const AABB& aabb, const Ray& ray) {
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max(); // 無限遠まで判定する

        // x軸
        if (ray.diff.x != 0.0f) {
            float tx1 = (aabb.min.x - ray.origin.x) / ray.diff.x;
            float tx2 = (aabb.max.x - ray.origin.x) / ray.diff.x;
            float tNearX = std::min(tx1, tx2);
            float tFarX = std::max(tx1, tx2);
            tMin = std::max(tMin, tNearX);
            tMax = std::min(tMax, tFarX);
        } else {
            // x軸が0ならRayはX方向に進まない ⇒ AABBのX範囲にoriginがないなら衝突なし
            if (ray.origin.x < aabb.min.x || ray.origin.x > aabb.max.x) {
                return false;
            }
        }

        // y軸
        if (ray.diff.y != 0.0f) {
            float ty1 = (aabb.min.y - ray.origin.y) / ray.diff.y;
            float ty2 = (aabb.max.y - ray.origin.y) / ray.diff.y;
            float tNearY = std::min(ty1, ty2);
            float tFarY = std::max(ty1, ty2);
            tMin = std::max(tMin, tNearY);
            tMax = std::min(tMax, tFarY);
        } else {
            if (ray.origin.y < aabb.min.y || ray.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (ray.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - ray.origin.z) / ray.diff.z;
            float tz2 = (aabb.max.z - ray.origin.z) / ray.diff.z;
            float tNearZ = std::min(tz1, tz2);
            float tFarZ = std::max(tz1, tz2);
            tMin = std::max(tMin, tNearZ);
            tMax = std::min(tMax, tFarZ);
        } else {
            if (ray.origin.z < aabb.min.z || ray.origin.z > aabb.max.z) {
                return false;
            }
        }

        // 衝突判定：tMin が tMax 以下 かつ tMax が正方向
        if ((tMin <= tMax) && (tMax >= 0.0f)) {
            return true;
        }

        return false;
    }

    // AABBと直線の衝突判定
    bool IsCollision(const AABB& aabb, const Line& line) {
        float tMin = -std::numeric_limits<float>::max(); // 無限負方向
        float tMax = std::numeric_limits<float>::max();  // 無限正方向

        // x軸
        if (line.diff.x != 0.0f) {
            float tx1 = (aabb.min.x - line.origin.x) / line.diff.x;
            float tx2 = (aabb.max.x - line.origin.x) / line.diff.x;
            float tNearX = std::min(tx1, tx2);
            float tFarX = std::max(tx1, tx2);
            tMin = std::max(tMin, tNearX);
            tMax = std::min(tMax, tFarX);
        } else {
            if (line.origin.x < aabb.min.x || line.origin.x > aabb.max.x) {
                return false;
            }
        }

        // y軸
        if (line.diff.y != 0.0f) {
            float ty1 = (aabb.min.y - line.origin.y) / line.diff.y;
            float ty2 = (aabb.max.y - line.origin.y) / line.diff.y;
            float tNearY = std::min(ty1, ty2);
            float tFarY = std::max(ty1, ty2);
            tMin = std::max(tMin, tNearY);
            tMax = std::min(tMax, tFarY);
        } else {
            if (line.origin.y < aabb.min.y || line.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (line.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - line.origin.z) / line.diff.z;
            float tz2 = (aabb.max.z - line.origin.z) / line.diff.z;
            float tNearZ = std::min(tz1, tz2);
            float tFarZ = std::max(tz1, tz2);
            tMin = std::max(tMin, tNearZ);
            tMax = std::min(tMax, tFarZ);
        } else {
            if (line.origin.z < aabb.min.z || line.origin.z > aabb.max.z) {
                return false;
            }
        }

        return tMin <= tMax;
    }

    // AABBと頂点の衝突判定
    bool IsCollision(const AABB& aabb, const Vector3& point) {

        if ((aabb.min.x <= point.x && aabb.max.x >= point.x) && // x軸
            (aabb.min.y <= point.y && aabb.max.y >= point.y) && // y軸
            (aabb.min.z <= point.z && aabb.max.z >= point.z)    // z軸
            ) {
            return true;
        }

        return false;
    }

    // 球と球の衝突判定
    bool IsSphereCollision(const Sphere& s1, const Sphere& s2) {
        // 2つの球の中心点間の距離を求める
        float distance = Math::Length(Math::Subtract(s2.center, s1.center));
        // 半径の合計よりも短ければ衝突
        if (distance <= s1.radius + s2.radius) {
            return true;
        }
        return false;
    }

    // 平面と球の衝突判定
    bool IsSpherePlaneCollision(const Sphere& sphere, const Plane& plane) {
        float k = std::fabs(Math::Dot(plane.normal, sphere.center) - plane.distance);
        if (k <= sphere.radius) {
            return true;
        }
        return false;
    }

    // 線分と平面の衝突判定
    bool IsSegmentPlaneCollision(const Segment& segment, const Plane& plane) {
        // まずは垂直判定を行うために、法線と線の内積を求める
        float dot = Math::Dot(plane.normal, segment.diff);
        // 垂直 = 平行であるので、衝突しているはずがない
        if (dot == 0.0f) {
            return false;
        }
        // tを求める
        float t = (plane.distance - Math::Dot(segment.origin, plane.normal)) / dot;
        // tの値と線の種類によって衝突しているかを判断する
        if (0.0f <= t && t <= 1.0f) {
            return true;
        }
        return false;
    }

    // 半直線と平面の衝突判定
    bool IsRayPlaneCollision(const Ray& ray, const Plane& plane) {
        // まずは垂直判定を行うために、法線と線の内積を求める
        float dot = Math::Dot(plane.normal, ray.diff);
        // 垂直 = 平行であるので、衝突しているはずがない
        if (dot == 0.0f) {
            return false;
        }
        // tを求める
        float t = (plane.distance - Math::Dot(ray.origin, plane.normal)) / dot;
        // tの値と線の種類によって衝突しているかを判断する
        if (0.0f <= t) {
            return true;
        }
        return false;
    }

    // 直線と平面の衝突判定
    bool IsLinePlaneCollision(const Line& line, const Plane& plane) {
        // まずは垂直判定を行うために、法線と線の内積を求める
        float dot = Math::Dot(plane.normal, line.diff);
        // 垂直 = 平行であるので、衝突しているはずがない
        if (dot == 0.0f) {
            return false;
        }
        return true;
    }

    // 三角形と線分の衝突判定
    bool IsTriangleSegmentCollision(const Triangle& triangle, const Segment& segment) {
        Vector3 normal = Math::Cross(Math::Subtract(triangle.vertices_[1], triangle.vertices_[0]), Math::Subtract(triangle.vertices_[2], triangle.vertices_[0]));
        float dot = Math::Dot(normal, segment.diff);
        if (dot == 0.0f) {
            return false;
        }
        float t = Math::Dot(normal, Math::Subtract(triangle.vertices_[0], segment.origin)) / dot;
        if (t < 0.0f || t > 1.0f) {
            return false;
        }
        Vector3 p = Math::Add(segment.origin, Math::Multiply(t, segment.diff));
        Vector3 v01 = Math::Subtract(triangle.vertices_[1], triangle.vertices_[0]);
        Vector3 v1p = Math::Subtract(p, triangle.vertices_[1]);
        Vector3 cross01 = Math::Cross(v01, v1p);
        Vector3 v12 = Math::Subtract(triangle.vertices_[2], triangle.vertices_[1]);
        Vector3 v2p = Math::Subtract(p, triangle.vertices_[2]);
        Vector3 cross12 = Math::Cross(v12, v2p);
        Vector3 v20 = Math::Subtract(triangle.vertices_[0], triangle.vertices_[2]);
        Vector3 v0p = Math::Subtract(p, triangle.vertices_[0]);
        Vector3 cross20 = Math::Cross(v20, v0p);
        if (Math::Dot(cross01, normal) >= 0.0f && Math::Dot(cross12, normal) >= 0.0f && Math::Dot(cross20, normal) >= 0.0f) {
            return true;
        }
        return false;
    }

    // AABBとAABBの衝突判定
    bool IsAABBCollision(const AABB& a, const AABB& b) {
        if ((a.min.x <= b.max.x && a.max.x >= b.min.x) && // x軸
            (a.min.y <= b.max.y && a.max.y >= b.min.y) && // y軸
            (a.min.z <= b.max.z && a.max.z >= b.min.z)    // z軸
            ) {
            return true;
        }
        return false;
    }

    // AABBと球の衝突判定
    bool IsAABBSphereCollision(const AABB& aabb, const Sphere& sphere) {
        Vector3 closestPoint{ std::clamp(sphere.center.x, aabb.min.x, aabb.max.x), std::clamp(sphere.center.y, aabb.min.y, aabb.max.y), std::clamp(sphere.center.z, aabb.min.z, aabb.max.z) };
        float distance = Math::Length(Math::Subtract(closestPoint, sphere.center));
        if (distance <= sphere.radius) {
            return true;
        }
        return false;
    }

    // AABBと線分の衝突判定
    bool IsAABBSegmentCollision(const AABB& aabb, const Segment& segment) {
        float tMin = 0.0f;
        float tMax = 1.0f;

        // x軸
        if (segment.diff.x != 0.0f) {
            float tx1 = (aabb.min.x - segment.origin.x) / segment.diff.x;
            float tx2 = (aabb.max.x - segment.origin.x) / segment.diff.x;
            float tNearX = std::min(tx1, tx2);
            float tFarX = std::max(tx1, tx2);
            tMin = std::max(tMin, tNearX);
            tMax = std::min(tMax, tFarX);
        } else {
            if (segment.origin.x < aabb.min.x || segment.origin.x > aabb.max.x) {
                return false;
            }
        }

        // y軸
        if (segment.diff.y != 0.0f) {
            float ty1 = (aabb.min.y - segment.origin.y) / segment.diff.y;
            float ty2 = (aabb.max.y - segment.origin.y) / segment.diff.y;
            float tNearY = std::min(ty1, ty2);
            float tFarY = std::max(ty1, ty2);
            tMin = std::max(tMin, tNearY);
            tMax = std::min(tMax, tFarY);
        } else {
            if (segment.origin.y < aabb.min.y || segment.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (segment.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - segment.origin.z) / segment.diff.z;
            float tz2 = (aabb.max.z - segment.origin.z) / segment.diff.z;
            float tNearZ = std::min(tz1, tz2);
            float tFarZ = std::max(tz1, tz2);
            tMin = std::max(tMin, tNearZ);
            tMax = std::min(tMax, tFarZ);
        } else {
            if (segment.origin.z < aabb.min.z || segment.origin.z > aabb.max.z) {
                return false;
            }
        }

        return tMin <= tMax;
    }

    // AABBと半直線の衝突判定
    bool IsAABBRayCollision(const AABB& aabb, const Ray& ray) {
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();

        // x軸
        if (ray.diff.x != 0.0f) {
            float tx1 = (aabb.min.x - ray.origin.x) / ray.diff.x;
            float tx2 = (aabb.max.x - ray.origin.x) / ray.diff.x;
            float tNearX = std::min(tx1, tx2);
            float tFarX = std::max(tx1, tx2);
            tMin = std::max(tMin, tNearX);
            tMax = std::min(tMax, tFarX);
        } else {
            if (ray.origin.x < aabb.min.x || ray.origin.x > aabb.max.x) {
                return false;
            }
        }

        // y軸
        if (ray.diff.y != 0.0f) {
            float ty1 = (aabb.min.y - ray.origin.y) / ray.diff.y;
            float ty2 = (aabb.max.y - ray.origin.y) / ray.diff.y;
            float tNearY = std::min(ty1, ty2);
            float tFarY = std::max(ty1, ty2);
            tMin = std::max(tMin, tNearY);
            tMax = std::min(tMax, tFarY);
        } else {
            if (ray.origin.y < aabb.min.y || ray.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (ray.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - ray.origin.z) / ray.diff.z;
            float tz2 = (aabb.max.z - ray.origin.z) / ray.diff.z;
            float tNearZ = std::min(tz1, tz2);
            float tFarZ = std::max(tz1, tz2);
            tMin = std::max(tMin, tNearZ);
            tMax = std::min(tMax, tFarZ);
        } else {
            if (ray.origin.z < aabb.min.z || ray.origin.z > aabb.max.z) {
                return false;
            }
        }

        return (tMin <= tMax) && (tMax >= 0.0f);
    }

    // AABBと直線の衝突判定
    bool IsAABBLineCollision(const AABB& aabb, const Line& line) {
        float tMin = -std::numeric_limits<float>::max();
        float tMax = std::numeric_limits<float>::max();

        // x軸
        if (line.diff.x != 0.0f) {
            float tx1 = (aabb.min.x - line.origin.x) / line.diff.x;
            float tx2 = (aabb.max.x - line.origin.x) / line.diff.x;
            float tNearX = std::min(tx1, tx2);
            float tFarX = std::max(tx1, tx2);
            tMin = std::max(tMin, tNearX);
            tMax = std::min(tMax, tFarX);
        } else {
            if (line.origin.x < aabb.min.x || line.origin.x > aabb.max.x) {
                return false;
            }
        }

        // y軸
        if (line.diff.y != 0.0f) {
            float ty1 = (aabb.min.y - line.origin.y) / line.diff.y;
            float ty2 = (aabb.max.y - line.origin.y) / line.diff.y;
            float tNearY = std::min(ty1, ty2);
            float tFarY = std::max(ty1, ty2);
            tMin = std::max(tMin, tNearY);
            tMax = std::min(tMax, tFarY);
        } else {
            if (line.origin.y < aabb.min.y || line.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (line.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - line.origin.z) / line.diff.z;
            float tz2 = (aabb.max.z - line.origin.z) / line.diff.z;
            float tNearZ = std::min(tz1, tz2);
            float tFarZ = std::max(tz1, tz2);
            tMin = std::max(tMin, tNearZ);
            tMax = std::min(tMax, tFarZ);
        } else {
            if (line.origin.z < aabb.min.z || line.origin.z > aabb.max.z) {
                return false;
            }
        }

        return tMin <= tMax;
    }

    // AABBと点の衝突判定
    bool IsAABBPointCollision(const AABB& aabb, const Vector3& point) {
        if ((aabb.min.x <= point.x && aabb.max.x >= point.x) && // x軸
            (aabb.min.y <= point.y && aabb.max.y >= point.y) && // y軸
            (aabb.min.z <= point.z && aabb.max.z >= point.z)    // z軸
            ) {
            return true;
        }
        return false;
    }

} // namespace Collision