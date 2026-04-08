#include "Collision.h"
#include "Math.h"
#include "AABB.h"
#include "OBB.h"
#include "Frustum.h"
#include "../../Shape/Plane.h"
#include "../../Shape/Sphere.h"
#include "../../Shape/Triangle.h"
#include "../../Shape/LinePrimitive.h"
#include "../Matrix4x4.h"
#include <array>
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

        // 平面と線分の内積(垂直＝平行チェック)
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
        Vector3 closestPoint{ Math::Clamp(sphere.center.x, aabb.min.x, aabb.max.x), Math::Clamp(sphere.center.y, aabb.min.y, aabb.max.y), Math::Clamp(sphere.center.z, aabb.min.z, aabb.max.z) };
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
            float tNearX = (std::min)(tx1, tx2);
            float tFarX = (std::max)(tx1, tx2);
            tMin = (std::max)(tMin, tNearX);
            tMax = (std::min)(tMax, tFarX);
        } else {
            if (segment.origin.x < aabb.min.x || segment.origin.x > aabb.max.x) {
                return false;
            }
        }

        // y軸
        if (segment.diff.y != 0.0f) {
            float ty1 = (aabb.min.y - segment.origin.y) / segment.diff.y;
            float ty2 = (aabb.max.y - segment.origin.y) / segment.diff.y;
            float tNearY = (std::min)(ty1, ty2);
            float tFarY = (std::max)(ty1, ty2);
            tMin = (std::max)(tMin, tNearY);
            tMax = (std::min)(tMax, tFarY);
        } else {
            if (segment.origin.y < aabb.min.y || segment.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (segment.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - segment.origin.z) / segment.diff.z;
            float tz2 = (aabb.max.z - segment.origin.z) / segment.diff.z;
            float tNearZ = (std::min)(tz1, tz2);
            float tFarZ = (std::max)(tz1, tz2);
            tMin = (std::max)(tMin, tNearZ);
            tMax = (std::min)(tMax, tFarZ);
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
        float tMax = (std::numeric_limits<float>::max)(); // 無限遠まで判定する

        // x軸
        if (ray.diff.x != 0.0f) {
            float tx1 = (aabb.min.x - ray.origin.x) / ray.diff.x;
            float tx2 = (aabb.max.x - ray.origin.x) / ray.diff.x;
            float tNearX = (std::min)(tx1, tx2);
            float tFarX = (std::max)(tx1, tx2);
            tMin = (std::max)(tMin, tNearX);
            tMax = (std::min)(tMax, tFarX);
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
            float tNearY = (std::min)(ty1, ty2);
            float tFarY = (std::max)(ty1, ty2);
            tMin = (std::max)(tMin, tNearY);
            tMax = (std::min)(tMax, tFarY);
        } else {
            if (ray.origin.y < aabb.min.y || ray.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (ray.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - ray.origin.z) / ray.diff.z;
            float tz2 = (aabb.max.z - ray.origin.z) / ray.diff.z;
            float tNearZ = (std::min)(tz1, tz2);
            float tFarZ = (std::max)(tz1, tz2);
            tMin = (std::max)(tMin, tNearZ);
            tMax = (std::min)(tMax, tFarZ);
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
        float tMin = -(std::numeric_limits<float>::max)(); // 無限負方向
        float tMax = (std::numeric_limits<float>::max)();  // 無限正方向

        // x軸
        if (line.diff.x != 0.0f) {
            float tx1 = (aabb.min.x - line.origin.x) / line.diff.x;
            float tx2 = (aabb.max.x - line.origin.x) / line.diff.x;
            float tNearX = (std::min)(tx1, tx2);
            float tFarX = (std::max)(tx1, tx2);
            tMin = (std::max)(tMin, tNearX);
            tMax = (std::min)(tMax, tFarX);
        } else {
            if (line.origin.x < aabb.min.x || line.origin.x > aabb.max.x) {
                return false;
            }
        }

        // y軸
        if (line.diff.y != 0.0f) {
            float ty1 = (aabb.min.y - line.origin.y) / line.diff.y;
            float ty2 = (aabb.max.y - line.origin.y) / line.diff.y;
            float tNearY = (std::min)(ty1, ty2);
            float tFarY = (std::max)(ty1, ty2);
            tMin = (std::max)(tMin, tNearY);
            tMax = (std::min)(tMax, tFarY);
        } else {
            if (line.origin.y < aabb.min.y || line.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (line.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - line.origin.z) / line.diff.z;
            float tz2 = (aabb.max.z - line.origin.z) / line.diff.z;
            float tNearZ = (std::min)(tz1, tz2);
            float tFarZ = (std::max)(tz1, tz2);
            tMin = (std::max)(tMin, tNearZ);
            tMax = (std::min)(tMax, tFarZ);
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

    // OBBと球の衝突判定
    bool IsCollision(const OBB& obb, const Sphere& sphere) {
        // 1. 球の中心点をOBBのローカル空間に変換する
        // OBBの中心から球の中心へのベクトル
        Vector3 worldRelPos = Math::Subtract(sphere.center, obb.center);

        // 各軸に射影してローカル座標を求める
        Vector3 localPos = {
            Math::Dot(worldRelPos, obb.orientations[0]),
            Math::Dot(worldRelPos, obb.orientations[1]),
            Math::Dot(worldRelPos, obb.orientations[2])
        };

        // 2. ローカル空間での「最近接点」を求める
        // OBBのローカル空間では、OBBは原点中心のAABBとして扱える
        // 範囲は [-size, size]
        Vector3 closestPoint = {
            Math::Clamp(localPos.x, -obb.size.x, obb.size.x),
            Math::Clamp(localPos.y, -obb.size.y, obb.size.y),
            Math::Clamp(localPos.z, -obb.size.z, obb.size.z)
        };

        // 3. ローカル空間での最近接点と球の中心(localPos)の距離を判定
        float distance = Math::Length(Math::Subtract(localPos, closestPoint));

        return distance <= sphere.radius;
    }

    // OBBと線分の衝突判定
    bool IsCollision(const OBB& obb, const Segment& segment) {
        // 1. 線分をOBBのローカル空間に変換する
        // OBBの中心からの相対座標
        Vector3 worldOriginRel = Math::Subtract(segment.origin, obb.center);

        // OBBの各軸(orientations)への射影を行い、ローカル空間の線分を作る
        Segment localSegment;
        localSegment.origin = {
            Math::Dot(worldOriginRel, obb.orientations[0]),
            Math::Dot(worldOriginRel, obb.orientations[1]),
            Math::Dot(worldOriginRel, obb.orientations[2])
        };
        localSegment.diff = {
            Math::Dot(segment.diff, obb.orientations[0]),
            Math::Dot(segment.diff, obb.orientations[1]),
            Math::Dot(segment.diff, obb.orientations[2])
        };

        // 2. ローカル空間でのAABBを作成
        // OBBはローカル空間では原点中心、サイズは obb.size の AABB となる
        AABB localAABB;
        localAABB.min = { -obb.size.x, -obb.size.y, -obb.size.z };
        localAABB.max = { obb.size.x,  obb.size.y,  obb.size.z };

        // 3. 既存の AABB と Segment の判定関数を呼び出す
        return IsAABBSegmentCollision(localAABB, localSegment);
    }

    // OBBとRay(半直線)の判定
    bool IsCollision(const OBB& obb, const Ray& ray) {
        // 1. RayをOBBのローカル空間に変換
        Vector3 worldRelPos = Math::Subtract(ray.origin, obb.center);
        Vector3 localOrigin = {
            Math::Dot(worldRelPos, obb.orientations[0]),
            Math::Dot(worldRelPos, obb.orientations[1]),
            Math::Dot(worldRelPos, obb.orientations[2])
        };
        Vector3 localDiff = {
            Math::Dot(ray.diff, obb.orientations[0]),
            Math::Dot(ray.diff, obb.orientations[1]),
            Math::Dot(ray.diff, obb.orientations[2])
        };

        // 2. スラブ法による判定
        float tMin = 0.0f; // Rayなので0以上
        float tMax = std::numeric_limits<float>::infinity();

        const float* originArr = &localOrigin.x;
        const float* diffArr = &localDiff.x;
        const float* sizeArr = &obb.size.x;

        for (int i = 0; i < 3; ++i) {
            // diffがほぼ0(線がこの軸に対して動いていない)場合
            if (std::abs(diffArr[i]) < 1e-6f) {
                // 始点がOBBの外側なら、平行なので一生当たらない
                if (std::abs(originArr[i]) > sizeArr[i]) return false;
            } else {
                // 各軸のスラブ(壁)との交差距離tを計算
                float t1 = (-sizeArr[i] - originArr[i]) / diffArr[i];
                float t2 = (sizeArr[i] - originArr[i]) / diffArr[i];

                float tNear = (std::min)(t1, t2);
                float tFar = (std::max)(t1, t2);

                tMin = (std::max)(tMin, tNear);
                tMax = (std::min)(tMax, tFar);
            }
        }

        // 最終的に重なった範囲があれば衝突
        return tMin <= tMax && tMax >= 0.0f;
    }

    // OBBとLine(直線)の判定
    bool IsCollision(const OBB& obb, const Line& line) {
        // 1. LineをOBBのローカル空間に変換(Rayと同様)
        Vector3 worldRelPos = Math::Subtract(line.origin, obb.center);
        Vector3 localOrigin = {
            Math::Dot(worldRelPos, obb.orientations[0]),
            Math::Dot(worldRelPos, obb.orientations[1]),
            Math::Dot(worldRelPos, obb.orientations[2])
        };
        Vector3 localDiff = {
            Math::Dot(line.diff, obb.orientations[0]),
            Math::Dot(line.diff, obb.orientations[1]),
            Math::Dot(line.diff, obb.orientations[2])
        };

        // 2. スラブ法(範囲制限なし)
        float tMin = -std::numeric_limits<float>::infinity();
        float tMax = std::numeric_limits<float>::infinity();

        const float* originArr = &localOrigin.x;
        const float* diffArr = &localDiff.x;
        const float* sizeArr = &obb.size.x;

        for (int i = 0; i < 3; ++i) {
            if (std::abs(diffArr[i]) < 1e-6f) {
                if (std::abs(originArr[i]) > sizeArr[i]) return false;
            } else {
                float t1 = (-sizeArr[i] - originArr[i]) / diffArr[i];
                float t2 = (sizeArr[i] - originArr[i]) / diffArr[i];
                tMin = (std::max)(tMin, (std::min)(t1, t2));
                tMax = (std::min)(tMax, (std::max)(t1, t2));
            }
        }

        return tMin <= tMax;
    }

    // OBBとOBBの衝突判定
    bool IsCollision(const OBB& a, const OBB& b) {
        // 2つのOBBの各軸(計6本)と、それらの外積(3x3=9本)の計15本を調べる

        // 1. 準備：回転行列(相対方向)と中心差分の計算
        float R[3][3], AbsR[3][3];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                // aの軸iとbの軸jの内積
                R[i][j] = Math::Dot(a.orientations[i], b.orientations[j]);
                // 絶対値(浮動小数点の誤差対策で僅かな値を加算)
                AbsR[i][j] = std::abs(R[i][j]) + 1e-6f;
            }
        }

        // 中心間の距離ベクトル
        Vector3 tWorld = Math::Subtract(b.center, a.center);
        // aのローカル座標系に変換
        Vector3 t = {
            Math::Dot(tWorld, a.orientations[0]),
            Math::Dot(tWorld, a.orientations[1]),
            Math::Dot(tWorld, a.orientations[2])
        };

        float ra, rb;

        // 2. 分離軸の判定(全15パターン)

        // --- aの各軸 (3本) ---
        for (int i = 0; i < 3; i++) {
            const float sizeA[] = { a.size.x, a.size.y, a.size.z };
            const float sizeB[] = { b.size.x, b.size.y, b.size.z };
            const float tArr[] = { t.x, t.y, t.z };

            ra = sizeA[i];
            rb = sizeB[0] * AbsR[i][0] + sizeB[1] * AbsR[i][1] + sizeB[2] * AbsR[i][2];
            if (std::abs(tArr[i]) > ra + rb) return false;
        }

        // --- bの各軸 (3本) ---
        for (int i = 0; i < 3; i++) {
            ra = a.size.x * AbsR[0][i] + a.size.y * AbsR[1][i] + a.size.z * AbsR[2][i];
            rb = (i == 0) ? b.size.x : (i == 1) ? b.size.y : b.size.z;
            float tRel = t.x * R[0][i] + t.y * R[1][i] + t.z * R[2][i];
            if (std::abs(tRel) > ra + rb) return false;
        }

        // --- aの各軸 x bの各軸 の外積 (9本) ---
        // L = A0 x B0
        ra = a.size.y * AbsR[2][0] + a.size.z * AbsR[1][0];
        rb = b.size.y * AbsR[0][2] + b.size.z * AbsR[0][1];
        if (std::abs(t.z * R[1][0] - t.y * R[2][0]) > ra + rb) return false;

        // L = A0 x B1
        ra = a.size.y * AbsR[2][1] + a.size.z * AbsR[1][1];
        rb = b.size.x * AbsR[0][2] + b.size.z * AbsR[0][0];
        if (std::abs(t.z * R[1][1] - t.y * R[2][1]) > ra + rb) return false;

        // L = A0 x B2
        ra = a.size.y * AbsR[2][2] + a.size.z * AbsR[1][2];
        rb = b.size.x * AbsR[0][1] + b.size.y * AbsR[0][0];
        if (std::abs(t.z * R[1][2] - t.y * R[2][2]) > ra + rb) return false;

        // L = A1 x B0
        ra = a.size.x * AbsR[2][0] + a.size.z * AbsR[0][0];
        rb = b.size.y * AbsR[1][2] + b.size.z * AbsR[1][1];
        if (std::abs(t.x * R[2][0] - t.z * R[0][0]) > ra + rb) return false;

        // L = A1 x B1
        ra = a.size.x * AbsR[2][1] + a.size.z * AbsR[0][1];
        rb = b.size.x * AbsR[1][2] + b.size.z * AbsR[1][0];
        if (std::abs(t.x * R[2][1] - t.z * R[0][1]) > ra + rb) return false;

        // L = A1 x B2
        ra = a.size.x * AbsR[2][2] + a.size.z * AbsR[0][2];
        rb = b.size.x * AbsR[1][1] + b.size.y * AbsR[1][0];
        if (std::abs(t.x * R[2][2] - t.z * R[0][2]) > ra + rb) return false;

        // L = A2 x B0
        ra = a.size.x * AbsR[1][0] + a.size.y * AbsR[0][0];
        rb = b.size.y * AbsR[2][2] + b.size.z * AbsR[2][1];
        if (std::abs(t.y * R[0][0] - t.x * R[1][0]) > ra + rb) return false;

        // L = A2 x B1
        ra = a.size.x * AbsR[1][1] + a.size.y * AbsR[0][1];
        rb = b.size.x * AbsR[2][2] + b.size.z * AbsR[2][0];
        if (std::abs(t.y * R[0][1] - t.x * R[1][1]) > ra + rb) return false;

        // L = A2 x B2
        ra = a.size.x * AbsR[1][2] + a.size.y * AbsR[0][2];
        rb = b.size.x * AbsR[2][1] + b.size.y * AbsR[2][0];
        if (std::abs(t.y * R[0][2] - t.x * R[1][2]) > ra + rb) return false;

        // すべての軸で重なっていたら衝突
        return true;
    }

    // OBBとAABBの衝突判定
    bool IsCollision(const OBB& obb, const AABB& aabb) {
        // AABBをOBBに変換して判定
        OBB aabbAsObb;
        aabbAsObb.center = {
            (aabb.min.x + aabb.max.x) * 0.5f,
            (aabb.min.y + aabb.max.y) * 0.5f,
            (aabb.min.z + aabb.max.z) * 0.5f
        };
        aabbAsObb.size = {
            (aabb.max.x - aabb.min.x) * 0.5f,
            (aabb.max.y - aabb.min.y) * 0.5f,
            (aabb.max.z - aabb.min.z) * 0.5f
        };
        aabbAsObb.orientations[0] = { 1.0f, 0.0f, 0.0f };
        aabbAsObb.orientations[1] = { 0.0f, 1.0f, 0.0f };
        aabbAsObb.orientations[2] = { 0.0f, 0.0f, 1.0f };

        return IsCollision(obb, aabbAsObb);
    }

    // 視錐台と球の衝突判定（カリング用）
    bool IsCollision(const Frustum& frustum, const Sphere& sphere) {
        // すべての平面に対して、球が外面（法線と反対側）に完全に出ていないかチェックする
        for (const auto& plane : frustum.planes) {
            // 平面方程式は Dot(N, P) - D = 0 (D = plane.distance)
            // 点Pの平面からの距離は Dot(N, P) - D
            // これが -sphere.radius より小さければ、球は平面の外側（法線と反対側）に完全にある
            if (Math::Dot(plane.normal, sphere.center) - plane.distance < -sphere.radius) {
                return false; // 1つでも平面の外側にあれば、視錐台の外
            }
        }
        return true; // すべての平面の内側、または境界と重なっている
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
        Vector3 closestPoint{ Math::Clamp(sphere.center.x, aabb.min.x, aabb.max.x), Math::Clamp(sphere.center.y, aabb.min.y, aabb.max.y), Math::Clamp(sphere.center.z, aabb.min.z, aabb.max.z) };
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
            float tNearX = (std::min)(tx1, tx2);
            float tFarX = (std::max)(tx1, tx2);
            tMin = (std::max)(tMin, tNearX);
            tMax = (std::min)(tMax, tFarX);
        } else {
            if (segment.origin.x < aabb.min.x || segment.origin.x > aabb.max.x) {
                return false;
            }
        }

        // y軸
        if (segment.diff.y != 0.0f) {
            float ty1 = (aabb.min.y - segment.origin.y) / segment.diff.y;
            float ty2 = (aabb.max.y - segment.origin.y) / segment.diff.y;
            float tNearY = (std::min)(ty1, ty2);
            float tFarY = (std::max)(ty1, ty2);
            tMin = (std::max)(tMin, tNearY);
            tMax = (std::min)(tMax, tFarY);
        } else {
            if (segment.origin.y < aabb.min.y || segment.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (segment.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - segment.origin.z) / segment.diff.z;
            float tz2 = (aabb.max.z - segment.origin.z) / segment.diff.z;
            float tNearZ = (std::min)(tz1, tz2);
            float tFarZ = (std::max)(tz1, tz2);
            tMin = (std::max)(tMin, tNearZ);
            tMax = (std::min)(tMax, tFarZ);
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
        float tMax = (std::numeric_limits<float>::max)();

        // x軸
        if (ray.diff.x != 0.0f) {
            float tx1 = (aabb.min.x - ray.origin.x) / ray.diff.x;
            float tx2 = (aabb.max.x - ray.origin.x) / ray.diff.x;
            float tNearX = (std::min)(tx1, tx2);
            float tFarX = (std::max)(tx1, tx2);
            tMin = (std::max)(tMin, tNearX);
            tMax = (std::min)(tMax, tFarX);
        } else {
            if (ray.origin.x < aabb.min.x || ray.origin.x > aabb.max.x) {
                return false;
            }
        }

        // y軸
        if (ray.diff.y != 0.0f) {
            float ty1 = (aabb.min.y - ray.origin.y) / ray.diff.y;
            float ty2 = (aabb.max.y - ray.origin.y) / ray.diff.y;
            float tNearY = (std::min)(ty1, ty2);
            float tFarY = (std::max)(ty1, ty2);
            tMin = (std::max)(tMin, tNearY);
            tMax = (std::min)(tMax, tFarY);
        } else {
            if (ray.origin.y < aabb.min.y || ray.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (ray.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - ray.origin.z) / ray.diff.z;
            float tz2 = (aabb.max.z - ray.origin.z) / ray.diff.z;
            float tNearZ = (std::min)(tz1, tz2);
            float tFarZ = (std::max)(tz1, tz2);
            tMin = (std::max)(tMin, tNearZ);
            tMax = (std::min)(tMax, tFarZ);
        } else {
            if (ray.origin.z < aabb.min.z || ray.origin.z > aabb.max.z) {
                return false;
            }
        }

        return (tMin <= tMax) && (tMax >= 0.0f);
    }

    // AABBと直線の衝突判定
    bool IsAABBLineCollision(const AABB& aabb, const Line& line) {
        float tMin = -(std::numeric_limits<float>::max)();
        float tMax = (std::numeric_limits<float>::max)();

        // x軸
        if (line.diff.x != 0.0f) {
            float tx1 = (aabb.min.x - line.origin.x) / line.diff.x;
            float tx2 = (aabb.max.x - line.origin.x) / line.diff.x;
            float tNearX = (std::min)(tx1, tx2);
            float tFarX = (std::max)(tx1, tx2);
            tMin = (std::max)(tMin, tNearX);
            tMax = (std::min)(tMax, tFarX);
        } else {
            if (line.origin.x < aabb.min.x || line.origin.x > aabb.max.x) {
                return false;
            }
        }

        // y軸
        if (line.diff.y != 0.0f) {
            float ty1 = (aabb.min.y - line.origin.y) / line.diff.y;
            float ty2 = (aabb.max.y - line.origin.y) / line.diff.y;
            float tNearY = (std::min)(ty1, ty2);
            float tFarY = (std::max)(ty1, ty2);
            tMin = (std::max)(tMin, tNearY);
            tMax = (std::min)(tMax, tFarY);
        } else {
            if (line.origin.y < aabb.min.y || line.origin.y > aabb.max.y) {
                return false;
            }
        }

        // z軸
        if (line.diff.z != 0.0f) {
            float tz1 = (aabb.min.z - line.origin.z) / line.diff.z;
            float tz2 = (aabb.max.z - line.origin.z) / line.diff.z;
            float tNearZ = (std::min)(tz1, tz2);
            float tFarZ = (std::max)(tz1, tz2);
            tMin = (std::max)(tMin, tNearZ);
            tMax = (std::min)(tMax, tFarZ);
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

    // OBBと球の衝突判定
    bool IsOBBSphereCollision(const OBB& obb, const Sphere& sphere) {
        return IsCollision(obb, sphere);
    }

    // OBBと線分の衝突判定
    bool IsOBBSegmentCollision(const OBB& obb, const Segment& segment) {
        return IsCollision(obb, segment);
    }

    // OBBとRay(半直線)の判定
    bool IsOBBRayCollision(const OBB& obb, const Ray& ray) {
        return IsCollision(obb, ray);
    }

    // OBBとLine(直線)の判定
    bool IsOBBLineCollision(const OBB& obb, const Line& line) {
        return IsCollision(obb, line);
    }

    // OBBとOBBの衝突判定
    bool IsOBBCollision(const OBB& a, const OBB& b) {
        return IsCollision(a, b);
    }

    // OBBとAABBの衝突判定
    bool IsOBBAABBCollision(const OBB& obb, const AABB& aabb) {
        return IsCollision(obb, aabb);
    }

} // namespace Collision