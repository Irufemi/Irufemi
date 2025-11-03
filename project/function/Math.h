#pragma once

#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../math/Matrix4x4.h"

//前方宣言
struct Segment2D;
struct Segment;
struct Ray;
struct Line;
struct Sphere;
struct Plane;
struct Triangle;
struct AABB;
struct OBB;

namespace Math {

#pragma region 2次元ベクトル関数

    // 加算
    Vector2 Add(const Vector2& a, const Vector2& b);

    // 減算
    Vector2 Subtract(const Vector2& a, const Vector2& b);

    // スカラー倍
    Vector2 Multiply(const float scalar, const Vector2 vector);

    // 内積
    float Dot(const Vector2& a, const Vector2& b);

    // ノルム(長さ)
    float Length(const Vector2& vector);

    // 正規化
    Vector2 Normalize(Vector2 vector);

    // 点と線分(3DのXY投影)の最近接点
    Vector2 ClosestPoint(const Vector2& point, const Segment2D& segment);

    Vector2 Bezier(const Vector2& p0, const Vector2& p1, const Vector2& p2, float t);

    Vector2 CatmullRom(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, float t);

#pragma endregion

#pragma region 3次元ベクトル関数

    /// <summary>
    /// 加算
    /// </summary>
    /// <param name="a"></param>
    /// <param name="b"></param>
    /// <returns></returns>
    Vector3 Add(const Vector3& a, const Vector3& b);

    /// <summary>
    /// 減算
    /// </summary>
    /// <param name="a"></param>
    /// <param name="b"></param>
    /// <returns></returns>
    Vector3 Subtract(const Vector3& a, const Vector3& b);

    /// <summary>
    /// スカラー倍
    /// </summary>
    /// <param name="scalar"></param>
    /// <param name="vector"></param>
    /// <returns></returns>
    Vector3 Multiply(const float scalar, const Vector3 vector);

    /// <summary>
    /// 内積
    /// </summary>
    /// <param name="a"></param>
    /// <param name="b"></param>
    /// <returns></returns>
    float Dot(const Vector3& a, const Vector3& b);


    /// <summary>
    /// ノルム(長さ)
    /// </summary>
    /// <param name="vector"></param>
    /// <returns></returns>
    float Length(const Vector3& vector);

    /// <summary>
    /// 正規化
    /// </summary>
    /// <param name="vector"></param>
    /// <returns></returns>
    Vector3 Normalize(const Vector3& vector);

    /// <summary>
    /// クロス積（外積）
    /// </summary>
    /// <param name="a"></param>
    /// <param name="b"></param>
    /// <returns></returns>
    Vector3 Cross(const Vector3& a, const Vector3& b);
    
    /// <summary>
    /// 正射影ベクトルを求める(v1をv2へ投影する(ベクトル射影))
    /// </summary>
    /// <param name="v1"></param>
    /// <param name="v2"></param>
    /// <returns></returns>
    Vector3 Project(const Vector3& v1, const Vector3& v2);

    /// <summary>
    /// 点と線分の距離を求める
    /// </summary>
    /// <param name="point"></param>
    /// <param name="segment"></param>
    /// <returns></returns>
    Vector3 ClosestPoint(const Vector3& point, const Segment& segment);

    /// <summary>
    /// 点と直線の距離を求める
    /// </summary>
    /// <param name="point"></param>
    /// <param name="ray"></param>
    /// <returns></returns>
    Vector3 ClosestPoint(const Vector3& point, const Ray& ray);

    /// <summary>
    /// 点と半直線の距離を求める
    /// </summary>
    /// <param name="point"></param>
    /// <param name="line"></param>
    /// <returns></returns>
    Vector3 ClosestPoint(const Vector3& point, const Line& line);

    //ベジェ曲線
    Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t);

    //スプライン曲線
    Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

#pragma endregion

#pragma region 4x4行列関数

    /// <summary>
    /// 4x4行列の加法
    /// </summary>
    /// <param name="m1"></param>
    /// <param name="m2"></param>
    /// <returns></returns>
    Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

    /// <summary>
    /// 4x4行列の減法
    /// </summary>
    /// <param name="m1"></param>
    /// <param name="m2"></param>
    /// <returns></returns>
    Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

    /// <summary>
    /// 4x4行列の積
    /// </summary>
    /// <param name="vector"></param>
    /// <param name="m"></param>
    /// <returns></returns>
    Matrix4x4 Multiply(const Matrix4x4& vector, const Matrix4x4& m);

    /// <summary>
    /// 4x4逆行列を求める
    /// </summary>
    /// <param name="m"></param>
    /// <returns></returns>
    Matrix4x4 Inverse(const Matrix4x4& m);

    /// <summary>
    /// 4x4転置行列を求める 
    /// </summary>
    /// <param name="m"></param>
    /// <returns></returns>
    Matrix4x4 Transpose(const Matrix4x4& m);

    /// <summary>
    /// 4x4単位行列の作成
    /// </summary>
    /// <returns></returns>
    Matrix4x4 MakeIdentity4x4();

    /// <summary>
    /// 4x4平行移動行列の作成
    /// </summary>
    /// <param name="translate"></param>
    /// <returns></returns>
    Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

    /// <summary>
    /// 4x4拡大縮小行列の作成
    /// </summary>
    /// <param name="scale"></param>
    /// <returns></returns>
    Matrix4x4 MakeScaleMatrix(const Vector3& scale);

    /// <summary>
    /// 3次元ベクトルを同次座標として変換
    /// </summary>
    /// <param name="vector"></param>
    /// <param name="matrix"></param>
    /// <returns></returns>
    Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);


    //回転は左手座標系(左手を握って親指の方向が軸、その他の指の幕方向が回転の正の方向)

    /// <summary>
    /// 4x4 X軸周り回転行列の作成
    /// </summary>
    /// <param name="theta"></param>
    /// <returns></returns>
    Matrix4x4 MakeRotateXMatrix(const float& theta);

    /// <summary>
    /// 4x4 Y軸周り回転行列の作成
    /// </summary>
    /// <param name="theta"></param>
    /// <returns></returns>
    Matrix4x4 MakeRotateYMatrix(const float& theta);

    /// <summary>
    /// 4x4 Z軸周り回転行列の作成
    /// </summary>
    /// <param name="theta"></param>
    /// <returns></returns>
    Matrix4x4 MakeRotateZMatrix(const float& theta);

    /// <summary>
    /// 4x4 3次元回転行列の作成
    /// </summary>
    /// <param name="thetaX"></param>
    /// <param name="thetaY"></param>
    /// <param name="thetaZ"></param>
    /// <returns></returns>
    Matrix4x4 MakeRotateXYZMatrix(const float& thetaX, const float& thetaY, const float& thetaZ);

    /// <summary>
    /// 4x4アフィン変換行列を高速に生成
    /// </summary>
    /// <param name="scale"></param>
    /// <param name="rotate"></param>
    /// <param name="translate"></param>
    /// <returns></returns>
    Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

    /// <summary>
    /// 透視投影行列の作成
    /// </summary>
    /// <param name="fovY"></param>
    /// <param name="aspectRatio"></param>
    /// <param name="nearClip"></param>
    /// <param name="farClip"></param>
    /// <returns></returns>
    Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

    /// <summary>
    /// 正射行列の作成
    /// </summary>
    /// <param name="left"></param>
    /// <param name="top"></param>
    /// <param name="right"></param>
    /// <param name="bottom"></param>
    /// <param name="nearClip"></param>
    /// <param name="farClip"></param>
    /// <returns></returns>
    Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

    /// <summary>
    /// ビューポート変換行列の作成
    /// </summary>
    /// <param name="left"></param>
    /// <param name="top"></param>
    /// <param name="width"></param>
    /// <param name="height"></param>
    /// <param name="minDepth"></param>
    /// <param name="maxDepth"></param>
    /// <returns></returns>
    Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

#pragma endregion

#pragma region 衝突判定

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

#pragma endregion
    Vector3 Perpendicular(const Vector3& vector);


}
