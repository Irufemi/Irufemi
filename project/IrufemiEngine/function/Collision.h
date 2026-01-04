#pragma once

// 前方宣言
struct Vector3;
struct Sphere;
struct Plane;
struct Segment;
struct Ray;
struct Line;
struct Triangle;
struct AABB;

namespace Collision {

    /// <summary>
    /// 球と球の衝突判定
    /// </summary>
    /// <param name="s1_center"></param>
    /// <param name="s1_radius"></param>
    /// <param name="s2_center"></param>
    /// <param name="s2_radius"></param>
    /// <returns></returns>
    bool IsCollision(const Vector3& s1_center, const float& s1_radius, const Vector3& s2_center, const float& s2_radius);

    /// <summary>
    /// 球と球の衝突判定
    /// </summary>
    /// <param name="s1"></param>
    /// <param name="s2"></param>
    /// <returns></returns>
    bool IsCollision(const Sphere& s1, const Sphere& s2);

    /// <summary>
    /// 球と平面の衝突判定
    /// </summary>
    /// <param name="sphere"></param>
    /// <param name="plane"></param>
    /// <returns></returns>
    bool IsCollision(const Sphere& sphere, const Plane& plane);

    /// <summary>
    /// 線分と平面の衝突判定
    /// </summary>
    /// <param name="segment"></param>
    /// <param name="plane"></param>
    /// <returns></returns>
    bool IsCollision(const Segment& segment, const Plane& plane);

    /// <summary>
    /// 半直線と平面の衝突判定
    /// </summary>
    /// <param name="ray"></param>
    /// <param name="plane"></param>
    /// <returns></returns>
    bool IsCollision(const Ray& ray, const Plane& plane);

    /// <summary>
    /// 直線と平面の衝突判定
    /// </summary>
    /// <param name="line"></param>
    /// <param name="plane"></param>
    /// <returns></returns>
    bool IsCollision(const Line& line, const Plane& plane);

    /// <summary>
    /// 三角形と線分の衝突判定
    /// </summary>
    /// <param name="triangle"></param>
    /// <param name="segment"></param>
    /// <returns></returns>
    bool IsCollision(const Triangle& triangle, const Segment& segment);

    /// <summary>
    /// AABBとAABBの衝突判定
    /// </summary>
    /// <param name="a"></param>
    /// <param name="b"></param>
    /// <returns></returns>
    bool IsCollision(const AABB& a, const AABB& b);

    /// <summary>
    /// AABBと球の衝突判定
    /// </summary>
    /// <param name="aabb"></param>
    /// <param name="sphere"></param>
    /// <returns></returns>
    bool IsCollision(const AABB& aabb, const Sphere& sphere);

    /// <summary>
    /// AABBと線分の衝突判定
    /// </summary>
    /// <param name="aabb"></param>
    /// <param name="plaane"></param>
    /// <returns></returns>
    bool IsCollision(const AABB& aabb, const Segment& segment);

    /// <summary>
    /// AABBと半直線の衝突判定
    /// </summary>
    /// <param name="aabb"></param>
    /// <param name="plaane"></param>
    /// <returns></returns>
    bool IsCollision(const AABB& aabb, const Ray& ray);

    /// <summary>
    /// AABBと直線の衝突判定
    /// </summary>
    /// <param name="aabb"></param>
    /// <param name="plaane"></param>
    /// <returns></returns>
    bool IsCollision(const AABB& aabb, const Line& line);

    /// <summary>
    /// AABBと頂点の衝突判定
    /// </summary>
    /// <param name="aabb"></param>
    /// <param name="point"></param>
    /// <returns></returns>
    bool IsCollision(const AABB& aabb, const Vector3& point);

    /// <summary>
    /// 球と球の衝突判定
    /// </summary>
    bool IsSphereCollision(const Sphere& s1, const Sphere& s2);

    /// <summary>
    /// 球と平面の衝突判定
    /// </summary>
    bool IsSpherePlaneCollision(const Sphere& sphere, const Plane& plane);

    /// <summary>
    /// 線分と平面の衝突判定
    /// </summary>
    bool IsSegmentPlaneCollision(const Segment& segment, const Plane& plane);

    /// <summary>
    /// 半直線と平面の衝突判定
    /// </summary>
    bool IsRayPlaneCollision(const Ray& ray, const Plane& plane);

    /// <summary>
    /// 直線と平面の衝突判定
    /// </summary>
    bool IsLinePlaneCollision(const Line& line, const Plane& plane);

    /// <summary>
    /// 三角形と線分の衝突判定
    /// </summary>
    bool IsTriangleSegmentCollision(const Triangle& triangle, const Segment& segment);

    /// <summary>
    /// AABBとAABBの衝突判定
    /// </summary>
    bool IsAABBCollision(const AABB& a, const AABB& b);

    /// <summary>
    /// AABBと球の衝突判定
    /// </summary>
    bool IsAABBSphereCollision(const AABB& aabb, const Sphere& sphere);

    /// <summary>
    /// AABBと線分の衝突判定
    /// </summary>
    bool IsAABBSegmentCollision(const AABB& aabb, const Segment& segment);

    /// <summary>
    /// AABBと半直線の衝突判定
    /// </summary>
    bool IsAABBRayCollision(const AABB& aabb, const Ray& ray);

    /// <summary>
    /// AABBと直線の衝突判定
    /// </summary>
    bool IsAABBLineCollision(const AABB& aabb, const Line& line);

    /// <summary>
    /// AABBと点の衝突判定
    /// </summary>
    bool IsAABBPointCollision(const AABB& aabb, const Vector3& point);

} // namespace Collision