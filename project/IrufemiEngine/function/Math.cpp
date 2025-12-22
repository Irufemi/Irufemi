#define NOMINMAX
#include "Math.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <algorithm> 
#include <limits> 

#include "Ease.h"
#include "math/shape/AABB.h"
#include "math/shape/LinePrimitive.h"
#include "math/shape/Plane.h"
#include "math/shape/Sphere.h"
#include "math/shape/Triangle.h"

namespace Math {

#pragma region 2次元ベクトル関数

    // 加算
    Vector2 Add(const Vector2& a, const Vector2& b) { return { a.x + b.x, a.y + b.y }; }

    // 減算
    Vector2 Subtract(const Vector2& a, const Vector2& b) { return { a.x - b.x, a.y - b.y }; }

    // スカラー倍
    Vector2 Multiply(const float scalar, const Vector2 vector) { return { vector.x * scalar, vector.y * scalar }; }

    // 内積
    float Dot(const Vector2& a, const Vector2& b) { return a.x * b.x + a.y * b.y; }

    // ノルム(長さ)
    float Length(const Vector2& vector) { return std::sqrt(Dot(vector, vector)); }

    // 正規化
    Vector2 Normalize(Vector2 vector) {
        float length = sqrtf(powf(vector.x, 2.0f) + powf(vector.y, 2.0f));
        if (length == 0.0f) {
            return { 0.0f, 0.0f };
        }
        // 長さが0の場合はゼロベクトルを返す
        Vector2 result = { vector.x / length, vector.y / length };
        return result;
    }

    // 点と線分(2D)の最近接点（Segment2D: endは終点座標）
    Vector2 ClosestPoint(const Vector2& point, const Segment2D& segment) {
        Vector2 ab = Subtract(segment.end, segment.origin);
        Vector2 ap = Subtract(point, segment.origin);

        float t = Dot(ap, ab) / (powf(ab.x, 2) + powf(ab.y, 2));

        if (t < 0) {
            return segment.origin;
        }
        if (t > 1) {
            return segment.end;
        } else {
            Vector2 projection = Add(segment.origin, Multiply(t, ab));
            return projection;
        }
    }

    // 2次ベジェ曲線上の点を求める関数
    Vector2 Bezier(const Vector2& p0, const Vector2& p1, const Vector2& p2, float t) {
        // 制御点p0,p1を線形補間
        Vector2 p0p1 = Lerp(p0, p1, t);
        // 制御点p1,p2を線形補間
        Vector2 p1p2 = Lerp(p1, p2, t);
        // 補間点p0p1, p1p2をさらに線形補間
        Vector2 p = Lerp(p0p1, p1p2, t);
        return p;
    }

    // Catmull-ronスプライン上の点を求める関数
    Vector2 CatmullRom(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, float t) {
        Vector2 p = { 0 };
        p.x = 1.0f / 2.0f * ((-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * powf(t, 3.0f) + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * powf(t, 2.0f) + (-1.0f * p0.x + p2.x) * t + 2.0f * p1.x);
        p.y = 1.0f / 2.0f * ((-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * powf(t, 3.0f) + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * powf(t, 2.0f) + (-1.0f * p0.y + p2.y) * t + 2.0f * p1.y);
        return p;
    }

#pragma endregion
#pragma region 3次元ベクトル関数

    //加算
    Vector3 Add(const Vector3& a, const Vector3& b) {

        return { a.x + b.x,a.y + b.y,a.z + b.z };

    }

    //減算
    Vector3 Subtract(const Vector3& a, const Vector3& b) {

        return { a.x - b.x,a.y - b.y,a.z - b.z };
    }

    //スカラー倍
    Vector3 Multiply(const float scalar, const Vector3 vector) {

        return { vector.x * scalar,vector.y * scalar,vector.z * scalar };
    }

    //内積(a ・ b)
    float Dot(const Vector3& a, const Vector3& b) {

        return { a.x * b.x + a.y * b.y + a.z * b.z };
    }

    //ノルム(長さ)( ||v|| )
    float Length(const Vector3& vector) {

        return std::sqrt(Dot(vector, vector));

    }

    //正規化
    Vector3 Normalize(const Vector3& vector) {

        return Multiply(1.0f / Length(vector), vector);

    }

    //クロス積（外積）
    Vector3 Cross(const Vector3& a, const Vector3& b) {

        return  { a.y * b.z - a.z * b.y,a.z * b.x - a.x * b.z,a.x * b.y - a.y * b.x };

    }


    // 正射影ベクトルを求める(v1をv2へ投影する(ベクトル射影))
    Vector3 Project(const Vector3& v1, const Vector3& v2) { return Multiply(Dot(v1, v2) / Dot(v2, v2), v2); }

    // 点と線分の距離を求める
    Vector3 ClosestPoint(const Vector3& point, const Segment& segment) {

        Vector3 a = Subtract(point, segment.origin);
        float t = Dot(a, segment.diff) / Dot(segment.diff, segment.diff);
        t = std::clamp(t, 0.0f, 1.0f);
        return Add(segment.origin, Multiply(t, segment.diff));
    }

    // 点と半直線の距離を求める
    Vector3 ClosestPoint(const Vector3& point, const Ray& ray) {
        Vector3 a = Subtract(point, ray.origin);
        float t = Dot(a, ray.diff) / Dot(ray.diff, ray.diff);
        t = std::max(t, 0.0f); // t < 0 の場合は始点が最も近い
        return Add(ray.origin, Multiply(t, ray.diff));
    }

    // 点と直線の距離を求める
    Vector3 ClosestPoint(const Vector3& point, const Line& line) {
        Vector3 a = Subtract(point, line.origin);
        float t = Dot(a, line.diff) / Dot(line.diff, line.diff);
        return Add(line.origin, Multiply(t, line.diff)); // tに制限なし（無限直線）
    }

    // ベジェ曲線
    Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t) {
        // 制御点p0,p1を線形補間
        Vector3 p0p1 = Lerp(p0, p1, t);
        // 制御点p1,p2を線形補間
        Vector3 p1p2 = Lerp(p1, p2, t);
        // 補間点p0p1, p1p2をさらに線形補間
        Vector3 p = Lerp(p0p1, p1p2, t);
        return p;
    }

    // スプライン曲線
    Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
        Vector3 p;
        float t2 = t * t;
        float t3 = t2 * t;

        p.x = 0.5f * ((-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3 + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + p2.x) * t + 2.0f * p1.x);
        p.y = 0.5f * ((-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3 + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + p2.y) * t + 2.0f * p1.y);
        p.z = 0.5f * ((-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3 + (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 + (-p0.z + p2.z) * t + 2.0f * p1.z);

        return p;
    }

#pragma endregion

#pragma region 4次元ベクトル関数
    Vector4 Add(const Vector4& a, const Vector4& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    }

    Vector4 Subtract(const Vector4& a, const Vector4& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    }

    Vector4 Multiply(float s, const Vector4& v)
    {
        return { v.x * s, v.y * s, v.z * s, v.w * s };
    }

    float Dot(const Vector4& a, const Vector4& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    float Length(const Vector4& v)
    {
        return std::sqrt(Dot(v, v));
    }

    Vector4 Normalize(const Vector4& v)
    {
        float len = Length(v);
        if (len != 0.0f) {
            return Multiply(1.0f / len, v);
        }
        return v;
    }
#pragma endregion


#pragma region 4x4行列関数

    // 4x4行列の加法
    Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2) {
        Matrix4x4 addResult{};

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                addResult.m[i][j] = m1.m[i][j] + m2.m[i][j];
            }
        }

        return addResult;
    }

    // 4x4行列の減法
    Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2) {
        Matrix4x4 subtractResult{};

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                subtractResult.m[i][j] = m1.m[i][j] - m2.m[i][j];
            }
        }

        return subtractResult;
    }

    // 4x4行列の積
    Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
        Matrix4x4 r{};

        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                r.m[row][col] =
                    m1.m[row][0] * m2.m[0][col] +
                    m1.m[row][1] * m2.m[1][col] +
                    m1.m[row][2] * m2.m[2][col] +
                    m1.m[row][3] * m2.m[3][col];
            }
        }

    return r;
    }

    // 4x4逆行列を求める 
    Matrix4x4 Inverse(const Matrix4x4& m) {
        Matrix4x4 inv{};

        const float* a = &m.m[0][0];
        float* o = &inv.m[0][0];

        o[0] = a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15] + a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
        o[1] = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15] - a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
        o[2] = a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15] + a[5] * a[3] * a[14] + a[13] * a[2] * a[7] - a[13] * a[3] * a[6];
        o[3] = -a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11] - a[5] * a[3] * a[10] - a[9] * a[2] * a[7] + a[9] * a[3] * a[6];
        o[4] = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15] - a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
        o[5] = a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15] + a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
        o[6] = -a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15] - a[4] * a[3] * a[14] - a[12] * a[2] * a[7] + a[12] * a[3] * a[6];
        o[7] = a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11] + a[4] * a[3] * a[10] + a[8] * a[2] * a[7] - a[8] * a[3] * a[6];
        o[8] = a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15] + a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
        o[9] = -a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15] - a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
        o[10] = a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15] + a[4] * a[3] * a[13] + a[12] * a[1] * a[7] - a[12] * a[3] * a[5];
        o[11] = -a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11] - a[4] * a[3] * a[9] - a[8] * a[1] * a[7] + a[8] * a[3] * a[5];
        o[12] = -a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14] - a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
        o[13] = a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14] + a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
        o[14] = -a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14] - a[4] * a[2] * a[13] - a[12] * a[1] * a[6] + a[12] * a[2] * a[5];
        o[15] = a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10] + a[4] * a[2] * a[9] + a[8] * a[1] * a[6] - a[8] * a[2] * a[5];

        float det = a[0] * o[0] + a[1] * o[4] + a[2] * o[8] + a[3] * o[12];

        if (det == 0.0f) {
            return Matrix4x4(); // 逆行列が存在しない場合（ゼロ行列返すなど）
        }

        float invDet = 1.0f / det;
        for (int i = 0; i < 16; ++i) {
            o[i] *= invDet;
        }

        return inv;
    }

    // 4x4転置行列を求める 
    Matrix4x4 Transpose(const Matrix4x4& m) {
        Matrix4x4 tMatrix{};
        tMatrix.m[0][0] = m.m[0][0];
        tMatrix.m[0][1] = m.m[1][0];
        tMatrix.m[0][2] = m.m[2][0];
        tMatrix.m[0][3] = m.m[3][0];
        tMatrix.m[1][0] = m.m[0][1];
        tMatrix.m[1][1] = m.m[1][1];
        tMatrix.m[1][2] = m.m[2][1];
        tMatrix.m[1][3] = m.m[3][1];
        tMatrix.m[2][0] = m.m[0][2];
        tMatrix.m[2][1] = m.m[1][2];
        tMatrix.m[2][2] = m.m[2][2];
        tMatrix.m[2][3] = m.m[3][2];
        tMatrix.m[3][0] = m.m[0][3];
        tMatrix.m[3][1] = m.m[1][3];
        tMatrix.m[3][2] = m.m[2][3];
        tMatrix.m[3][3] = m.m[3][3];
        return tMatrix;
    }

    //4x4単位行列の作成
    Matrix4x4 MakeIdentity4x4() {
        Matrix4x4 m = {
            1.0f,0.0f,0.0f,0.0f,
            0.0f,1.0f,0.0f,0.0f,
            0.0f,0.0f,1.0f,0.0f,
            0.0f,0.0f,0.0f,1.0f
        };
        return m;
    }

    // 4x4平行移動行列の作成関数 
    Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
        Matrix4x4 resultTranslateMatrix{};
        resultTranslateMatrix.m[0][0] = 1.0f;
        resultTranslateMatrix.m[0][1] = 0.0f;
        resultTranslateMatrix.m[0][2] = 0.0f;
        resultTranslateMatrix.m[0][3] = 0.0f;
        resultTranslateMatrix.m[1][0] = 0.0f;
        resultTranslateMatrix.m[1][1] = 1.0f;
        resultTranslateMatrix.m[1][2] = 0.0f;
        resultTranslateMatrix.m[1][3] = 0.0f;
        resultTranslateMatrix.m[2][0] = 0.0f;
        resultTranslateMatrix.m[2][1] = 0.0f;
        resultTranslateMatrix.m[2][2] = 1.0f;
        resultTranslateMatrix.m[2][3] = 0.0f;
        resultTranslateMatrix.m[3][0] = translate.x;
        resultTranslateMatrix.m[3][1] = translate.y;
        resultTranslateMatrix.m[3][2] = translate.z;
        resultTranslateMatrix.m[3][3] = 1.0f;
        return resultTranslateMatrix;
    }

    //4x4拡大縮小行列の作成関数
    Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
        Matrix4x4 resultScaleMtrix{};
        resultScaleMtrix.m[0][0] = scale.x;
        resultScaleMtrix.m[0][1] = 0.0f;
        resultScaleMtrix.m[0][2] = 0.0f;
        resultScaleMtrix.m[0][3] = 0.0f;
        resultScaleMtrix.m[1][0] = 0.0f;
        resultScaleMtrix.m[1][1] = scale.y;
        resultScaleMtrix.m[1][2] = 0.0f;
        resultScaleMtrix.m[1][3] = 0.0f;
        resultScaleMtrix.m[2][0] = 0.0f;
        resultScaleMtrix.m[2][1] = 0.0f;
        resultScaleMtrix.m[2][2] = scale.z;
        resultScaleMtrix.m[2][3] = 0.0f;
        resultScaleMtrix.m[3][0] = 0.0f;
        resultScaleMtrix.m[3][1] = 0.0f;
        resultScaleMtrix.m[3][2] = 0.0f;
        resultScaleMtrix.m[3][3] = 1.0f;
        return resultScaleMtrix;
    }

    // 3次元ベクトルを同次座標として変換する 
    Vector3 Transform(const Vector3& vector, const Matrix4x4& m) {
        Vector3 transformResult{};
        transformResult.x = vector.x * m.m[0][0] + vector.y * m.m[1][0] + vector.z * m.m[2][0] + 1.0f * m.m[3][0];
        transformResult.y = vector.x * m.m[0][1] + vector.y * m.m[1][1] + vector.z * m.m[2][1] + 1.0f * m.m[3][1];
        transformResult.z = vector.x * m.m[0][2] + vector.y * m.m[1][2] + vector.z * m.m[2][2] + 1.0f * m.m[3][2];
        float w = vector.x * m.m[0][3] + vector.y * m.m[1][3] + vector.z * m.m[2][3] + 1.0f * m.m[3][3];
        if (w != 0.0f) { ///ベクトルに対して基本的な操作を行う行列でwが0になることはありえない
            transformResult.x /= w; //w=1がデカルト座標系であるので、w除算することで同時座標をデカルト座標に戻す
            transformResult.y /= w;
            transformResult.z /= w;
            return transformResult;
        }
        else {
            return { 0 };
        }
    }

    // 4x4 X軸周り回転行列の作成関数
    Matrix4x4 MakeRotateXMatrix(const float& radian) {
        Matrix4x4 matrix{};
        matrix.m[0][0] = 1.0f;
        matrix.m[0][1] = 0.0f;
        matrix.m[0][2] = 0.0f;
        matrix.m[0][3] = 0.0f;
        matrix.m[1][0] = 0.0f;
        matrix.m[1][1] = std::cos(radian);
        matrix.m[1][2] = std::sin(radian);
        matrix.m[1][3] = 0.0f;
        matrix.m[2][0] = 0.0f;
        matrix.m[2][1] = -std::sin(radian);
        matrix.m[2][2] = std::cos(radian);
        matrix.m[2][3] = 0.0f;
        matrix.m[3][0] = 0.0f;
        matrix.m[3][1] = 0.0f;
        matrix.m[3][2] = 0.0f;
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    // 4x4 Y軸周り回転行列の作成関数
    Matrix4x4 MakeRotateYMatrix(const float& radian) {
        Matrix4x4 matrix{};
        matrix.m[0][0] = std::cos(radian);
        matrix.m[0][1] = 0.0f;
        matrix.m[0][2] = -std::sin(radian);
        matrix.m[0][3] = 0.0f;
        matrix.m[1][0] = 0.0f;
        matrix.m[1][1] = 1.0f;
        matrix.m[1][2] = 0.0f;
        matrix.m[1][3] = 0.0f;
        matrix.m[2][0] = std::sin(radian);
        matrix.m[2][1] = 0.0f;
        matrix.m[2][2] = std::cos(radian);
        matrix.m[2][3] = 0.0f;
        matrix.m[3][0] = 0.0f;
        matrix.m[3][1] = 0.0f;
        matrix.m[3][2] = 0.0f;
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    // 4x4 Z軸周り回転行列の作成関数
    Matrix4x4 MakeRotateZMatrix(const float& radian) {
        Matrix4x4 matrix{};
        matrix.m[0][0] = std::cos(radian);
        matrix.m[0][1] = std::sin(radian);
        matrix.m[0][2] = 0.0f;
        matrix.m[0][3] = 0.0f;
        matrix.m[1][0] = -std::sin(radian);
        matrix.m[1][1] = std::cos(radian);
        matrix.m[1][2] = 0.0f;
        matrix.m[1][3] = 0.0f;
        matrix.m[2][0] = 0.0f;
        matrix.m[2][1] = 0.0f;
        matrix.m[2][2] = 1.0f;
        matrix.m[2][3] = 0.0f;
        matrix.m[3][0] = 0.0f;
        matrix.m[3][1] = 0.0f;
        matrix.m[3][2] = 0.0f;
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    // 3次元回転行列の作成関数
    Matrix4x4 MakeRotateXYZMatrix(const float& thetaX, const float& thetaY, const float& thetaZ) {
        Matrix4x4 matrix = Multiply(MakeRotateXMatrix(thetaX), Multiply(MakeRotateYMatrix(thetaY), MakeRotateZMatrix(thetaZ)));
        return matrix;
    }

    //4x4アフィン変換行列を高速に生成
    Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
        Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(rotate.x, rotate.y, rotate.z);
        Matrix4x4 affineMatrix{};
        affineMatrix.m[0][0] = scale.x * rotateMatrix.m[0][0];
        affineMatrix.m[0][1] = scale.x * rotateMatrix.m[0][1];
        affineMatrix.m[0][2] = scale.x * rotateMatrix.m[0][2];
        affineMatrix.m[0][3] = 0.0f;
        affineMatrix.m[1][0] = scale.y * rotateMatrix.m[1][0];
        affineMatrix.m[1][1] = scale.y * rotateMatrix.m[1][1];
        affineMatrix.m[1][2] = scale.y * rotateMatrix.m[1][2];
        affineMatrix.m[1][3] = 0.0f;
        affineMatrix.m[2][0] = scale.z * rotateMatrix.m[2][0];
        affineMatrix.m[2][1] = scale.z * rotateMatrix.m[2][1];
        affineMatrix.m[2][2] = scale.z * rotateMatrix.m[2][2];
        affineMatrix.m[2][3] = 0.0f;
        affineMatrix.m[3][0] = translate.x;
        affineMatrix.m[3][1] = translate.y;
        affineMatrix.m[3][2] = translate.z;
        affineMatrix.m[3][3] = 1.0f;
        return affineMatrix;
    }

    //4x4アフィン変換行列を高速に生成 (Quaternion 回転版)
    Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotateQuaternion, const Vector3& translate) {
        Matrix4x4 rotateMatrix = MakeRotateMatrix(rotateQuaternion);
        Matrix4x4 affineMatrix{};
        affineMatrix.m[0][0] = scale.x * rotateMatrix.m[0][0];
        affineMatrix.m[0][1] = scale.x * rotateMatrix.m[0][1];
        affineMatrix.m[0][2] = scale.x * rotateMatrix.m[0][2];
        affineMatrix.m[0][3] = 0.0f;
        affineMatrix.m[1][0] = scale.y * rotateMatrix.m[1][0];
        affineMatrix.m[1][1] = scale.y * rotateMatrix.m[1][1];
        affineMatrix.m[1][2] = scale.y * rotateMatrix.m[1][2];
        affineMatrix.m[1][3] = 0.0f;
        affineMatrix.m[2][0] = scale.z * rotateMatrix.m[2][0];
        affineMatrix.m[2][1] = scale.z * rotateMatrix.m[2][1];
        affineMatrix.m[2][2] = scale.z * rotateMatrix.m[2][2];
        affineMatrix.m[2][3] = 0.0f;
        affineMatrix.m[3][0] = translate.x;
        affineMatrix.m[3][1] = translate.y;
        affineMatrix.m[3][2] = translate.z;
        affineMatrix.m[3][3] = 1.0f;
        return affineMatrix;
    }

    //透視投影行列の作成
    Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
        Matrix4x4 perspectiveFovMatrix{};
        perspectiveFovMatrix.m[0][0] = 1.0f / aspectRatio * (1.0f / std::tan(fovY / 2.0f));
        perspectiveFovMatrix.m[0][1] = 0.0f;
        perspectiveFovMatrix.m[0][2] = 0.0f;
        perspectiveFovMatrix.m[0][3] = 0.0f;
        perspectiveFovMatrix.m[1][0] = 0.0f;
        perspectiveFovMatrix.m[1][1] = 1.0f / std::tan(fovY / 2.0f);
        perspectiveFovMatrix.m[1][2] = 0.0f;
        perspectiveFovMatrix.m[1][3] = 0.0f;
        perspectiveFovMatrix.m[2][0] = 0.0f;
        perspectiveFovMatrix.m[2][1] = 0.0f;
        perspectiveFovMatrix.m[2][2] = farClip / (farClip - nearClip);
        perspectiveFovMatrix.m[2][3] = 1.0f;
        perspectiveFovMatrix.m[3][0] = 0.0f;
        perspectiveFovMatrix.m[3][1] = 0.0f;
        perspectiveFovMatrix.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);
        perspectiveFovMatrix.m[3][3] = 0.0f;
        return perspectiveFovMatrix;
    }

    //正射行列の作成
    Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float zNear, float zFar) {
        Matrix4x4 projectionMatrix{};
        projectionMatrix.m[0][0] = 2.0f / (right - left);
        projectionMatrix.m[0][1] = 0.0f;
        projectionMatrix.m[0][2] = 0.0f;
        projectionMatrix.m[0][3] = 0.0f;
        projectionMatrix.m[1][0] = 0.0f;
        projectionMatrix.m[1][1] = 2.0f / (top - bottom);
        projectionMatrix.m[1][2] = 0.0f;
        projectionMatrix.m[1][3] = 0.0f;
        projectionMatrix.m[2][0] = 0.0f;
        projectionMatrix.m[2][1] = 0.0f;
        projectionMatrix.m[2][2] = 1.0f / (zFar - zNear);
        projectionMatrix.m[2][3] = 0.0f;
        projectionMatrix.m[3][0] = (left + right) / (left - right);
        projectionMatrix.m[3][1] = (top + bottom) / (bottom - top);
        projectionMatrix.m[3][2] = zNear / (zNear - zFar);
        projectionMatrix.m[3][3] = 1.0f;
        return projectionMatrix;
    }

    //ビューポート変換行列の作成
    Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
        Matrix4x4 viewportMatrix{};
        viewportMatrix.m[0][0] = width / 2.0f;
        viewportMatrix.m[0][1] = 0.0f;
        viewportMatrix.m[0][2] = 0.0f;
        viewportMatrix.m[0][3] = 0.0f;
        viewportMatrix.m[1][0] = 0.0f;
        viewportMatrix.m[1][1] = -height / 2.0f;
        viewportMatrix.m[1][2] = 0.0f;
        viewportMatrix.m[1][3] = 0.0f;
        viewportMatrix.m[2][0] = 0.0f;
        viewportMatrix.m[2][1] = 0.0f;
        viewportMatrix.m[2][2] = maxDepth - minDepth;
        viewportMatrix.m[2][3] = 0.0f;
        viewportMatrix.m[3][0] = left + width / 2.0f;
        viewportMatrix.m[3][1] = top + height / 2.0f;
        viewportMatrix.m[3][2] = minDepth;
        viewportMatrix.m[3][3] = 1.0f;
        return viewportMatrix;
    }
    
    // 任意軸回転行列の作成関数
    Matrix4x4 MakeRotateAxisAngle(const Vector3& axis, float angle) {

        Matrix4x4 result = MakeIdentity4x4();
        result.m[0][0] = axis.x * axis.x * (1 - std::cos(angle)) + std::cos(angle);
        result.m[0][1] = axis.x * axis.y * (1 - std::cos(angle)) + axis.z * std::sin(angle);
        result.m[0][2] = axis.x * axis.z * (1 - std::cos(angle)) - axis.y * std::sin(angle);
        result.m[1][0] = axis.x * axis.y * (1 - std::cos(angle)) - axis.z * std::sin(angle);
        result.m[1][1] = axis.y * axis.y * (1 - std::cos(angle)) + std::cos(angle);
        result.m[1][2] = axis.y * axis.z * (1 - std::cos(angle)) + axis.x * std::sin(angle);
        result.m[2][0] = axis.x * axis.z * (1 - std::cos(angle)) + axis.y * std::sin(angle);
        result.m[2][1] = axis.y * axis.z * (1 - std::cos(angle)) - axis.x * std::sin(angle);
        result.m[2][2] = axis.z * axis.z * (1 - std::cos(angle)) + std::cos(angle);

        return result;

    }

    // ある方向からある方向への回転
    Matrix4x4 DirectionToDirection(const Vector3& from, const Vector3& to) {
        // 正規化
        Vector3 f = Normalize(from);
        Vector3 t = Normalize(to);

        // 内積から角度のコサインを求める
        float cosTheta = Dot(f, t);

        // 数値誤差対策の閾値
        constexpr float kEpsilon = 1e-6f;

        // ほぼ同じ方向 -> 単位行列（回転不要）
        if (cosTheta > 1.0f - kEpsilon) {
            return MakeIdentity4x4();
        }

        // ほぼ逆方向 -> 軸が定義できない（u x v = 0）になるので、
        // 添付画像の選び方に従って直交ベクトルを選ぶ
        if (cosTheta < -1.0f + kEpsilon) {
            Vector3 axis{ 0.0f, 0.0f, 0.0f };

            // 画像の式:
            // n = [ uy, -ux, 0 ]  (if ux != 0 || uy != 0)
            // n = [ uz, 0, -ux ]  (if ux != 0 || uz != 0)
            // 優先は前者（xy 平面での成分があれば使う）
            if (std::fabs(f.x) > kEpsilon || std::fabs(f.y) > kEpsilon) {
                axis = Vector3{ f.y, -f.x, 0.0f };
            } else if (std::fabs(f.x) > kEpsilon || std::fabs(f.z) > kEpsilon) {
                axis = Vector3{ f.z, 0.0f, -f.x };
            } else {
                // ほとんどゼロベクトルなら任意の軸を選択
                axis = Vector3{ 1.0f, 0.0f, 0.0f };
            }

            // 正規化（安全）
            float axisLen = Length(axis);
            if (axisLen < kEpsilon) {
                axis = Vector3{ 1.0f, 0.0f, 0.0f };
            } else {
                axis = Multiply(1.0f / axisLen, axis);
            }

            constexpr float kPi = 3.14159265358979323846f;
            return MakeRotateAxisAngle(axis, kPi);
        }

        // 通常ケース：軸は cross、角度は acos(dot)
        Vector3 axis = Cross(f, t);
        float axisLen = Length(axis);
        if (axisLen < kEpsilon) {
            // 数値誤差などで axis が小さい場合は単位行列を返す
            return MakeIdentity4x4();
        }
        axis = Multiply(1.0f / axisLen, axis);

        // acos の引数は [-1, 1] にクランプしておく
        float cosClamped = cosTheta;
        if (cosClamped < -1.0f) cosClamped = -1.0f;
        if (cosClamped > 1.0f) cosClamped = 1.0f;
        float angle = std::acos(cosClamped);

        return MakeRotateAxisAngle(axis, angle);
    }

    // 回転行列からオイラー角を抽出
    Vector3 ExtractEulerFromMatrix(const Matrix4x4& matrix) {
        Vector3 euler{};
        float sy = std::sqrt(matrix.m[0][0] * matrix.m[0][0] + matrix.m[1][0] * matrix.m[1][0]);
        constexpr float kEpsilon = 1e-6f;

        if (sy > kEpsilon) {
            euler.x = std::atan2(matrix.m[2][1], matrix.m[2][2]);
            euler.y = std::atan2(-matrix.m[2][0], sy);
            euler.z = std::atan2(matrix.m[1][0], matrix.m[0][0]);
        } else {
            // ジンバルロック状態
            euler.x = std::atan2(-matrix.m[1][2], matrix.m[1][1]);
            euler.y = std::atan2(-matrix.m[2][0], sy);
            euler.z = 0;
        }
        return euler;
    }


#pragma endregion

#pragma region Quaternion

    // Quaternionの積
    Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs) {

        Vector3 qlV = { lhs.x,lhs.y,lhs.z };
        float qlW = lhs.w;
        Vector3 qrV = { rhs.x,rhs.y,rhs.z };
        float qrW = rhs.w;

        float qW = qlW * qrW - Dot(qlV, qrV);
        Vector3 qV = Cross(qlV, qrV) + Multiply(qrW, qlV) + Multiply(qlW, qrV);

        Quaternion result = { qV.x,qV.y,qV.z,qW };

        return result;
    }

    // 単位Quaternionを返す
    Quaternion IdentityQuaternion() {
        Quaternion result{};
        result.x = 0.0f;
        result.y = 0.0f;
        result.z = 0.0f;
        result.w = 1.0f;
        return result;
    }

    // 共役Quaternionを返す
    Quaternion Conjugate(const Quaternion& quaternion) {
        Quaternion result{};
        // スカラー部はそのまま、虚部は符号を反転
        result.x = -quaternion.x;
        result.y = -quaternion.y;
        result.z = -quaternion.z;
        result.w = quaternion.w;
        return result;
    }

    // Quaternionのnormを返す
    float Norm(const Quaternion& quaternion) {
        float norm = std::sqrt(
            quaternion.w * quaternion.w +
            quaternion.x * quaternion.x +
            quaternion.y * quaternion.y +
            quaternion.z * quaternion.z
        );
        return norm;
    }

    // 正規化したQuaternionを返す
    Quaternion Normalize(const Quaternion& quaternion) {
        constexpr float kEpsilon = 1e-6f;
        float n = Norm(quaternion);
        if (n < kEpsilon) {
            // 長さがほぼ0の場合は元の値をそのまま返す（分母ゼロ回避）
            return quaternion;
        }
        Quaternion result{};
        result.x = quaternion.x / n;
        result.y = quaternion.y / n;
        result.z = quaternion.z / n;
        result.w = quaternion.w / n;
        return result;
    }

    // 逆Quaternionを返す
    Quaternion Inverse(const Quaternion& quaternion) {
        // 逆元は共役をノルム二乗で割る: q^{-1} = q* / ||q||^2
        constexpr float kEpsilon = 1e-12f;
        // ノルム二乗を直接計算（sqrt を使わず効率的）
        float normSq =
            quaternion.w * quaternion.w +
            quaternion.x * quaternion.x +
            quaternion.y * quaternion.y +
            quaternion.z * quaternion.z;

        if (normSq < kEpsilon) {
            // ノルム二乗がほぼ0なら逆元は定義されないため単位四元数を返す
            return IdentityQuaternion();
        }

        Quaternion conj = Conjugate(quaternion);
        Quaternion result{};
        result.x = conj.x / normSq;
        result.y = conj.y / normSq;
        result.z = conj.z / normSq;
        result.w = conj.w / normSq;
        return result;
    }

    // 任意軸回転を表す Quaternionの生成
    Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle) {
        constexpr float kEpsilon = 1e-6f;

        // 軸を正規化（ゼロ長なら X 軸を代替）
        float axisLen = Length(axis);
        Vector3 n = axis;
        if (axisLen < kEpsilon) {
            n = Vector3{ 1.0f, 0.0f, 0.0f };
        } else {
            n = Multiply(1.0f / axisLen, axis);
        }

        // 四元数 (x,y,z,w) : (n * sin(theta/2), cos(theta/2))
        float half = 0.5f * angle;
        float s = std::sin(half);
        Quaternion q = { n.x * s, n.y * s, n.z * s, std::cos(half) };

        // 数値安定性のため正規化して返す
        return Normalize(q);
    }

    // ベクトルをQuaternionで回転させた結果のベクトルを求める
    Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion) {
        // 回転四元数は単位四元数であるべきなので正規化して使用
        Quaternion q = Normalize(quaternion);

        // v を (v, 0) として q * v * q^*
        Quaternion p = { vector.x, vector.y, vector.z, 0.0f };
        Quaternion tmp = Multiply(q, p);
        Quaternion qConj = Conjugate(q);
        Quaternion res = Multiply(tmp, qConj);

        return Vector3{ res.x, res.y, res.z };
    }

    // Quaternionから回転行列を求める
    Matrix4x4 MakeRotateMatrix(const Quaternion& quaternion) {

        Matrix4x4 r = MakeIdentity4x4();

        r.m[0][0] = quaternion.w * quaternion.w + quaternion.x * quaternion.x - quaternion.y * quaternion.y - quaternion.z * quaternion.z;
        r.m[0][1] = 2.0f * (quaternion.x * quaternion.y + quaternion.w * quaternion.z);
        r.m[0][2] = 2.0f * (quaternion.x * quaternion.z - quaternion.w * quaternion.y);
        r.m[1][0] = 2.0f * (quaternion.x * quaternion.y - quaternion.w * quaternion.z);
        r.m[1][1] = quaternion.w * quaternion.w - quaternion.x * quaternion.x + quaternion.y * quaternion.y - quaternion.z * quaternion.z;
        r.m[1][2] = 2.0f * (quaternion.y * quaternion.z + quaternion.w * quaternion.x);
        r.m[2][0] = 2.0f * (quaternion.x * quaternion.z + quaternion.w * quaternion.y);
        r.m[2][1] = 2.0f * (quaternion.y * quaternion.z - quaternion.w * quaternion.x);
        r.m[2][2] = quaternion.w * quaternion.w - quaternion.x * quaternion.x - quaternion.y * quaternion.y + quaternion.z * quaternion.z;

        return r;
    }

    // 球面線形補間
    Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t) {
        // q0, q1 は単位四元数であることが前提（数値誤差は別途管理）
        Quaternion q0_ = q0;

        // q0 と q1 の内積（4次元ベクトルとして）
        float dot = q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;

        // 最短経路を取るために内積が負の場合は片方の符号を反転する
        if (dot < 0.0f) {
            q0_.x = -q0.x;
            q0_.y = -q0.y;
            q0_.z = -q0.z;
            q0_.w = -q0.w;
            dot = -dot;
        }

        // dot がほぼ 1 の場合は線形補間（数値安定化）
        constexpr float kDotThreshold = 0.9995f;
        if (dot > kDotThreshold) {
            // NLERP（線形補間）: cost を抑えるため正規化は呼び出し側で必要に応じて行う
            Quaternion result{};
            result.x = q0_.x + t * (q1.x - q0_.x);
            result.y = q0_.y + t * (q1.y - q0_.y);
            result.z = q0_.z + t * (q1.z - q0_.z);
            result.w = q0_.w + t * (q1.w - q0_.w);
            return result;
        }

        // acos の引数を安全にクランプ
        if (dot < -1.0f) dot = -1.0f;
        if (dot > 1.0f) dot = 1.0f;

        // なす角 theta を求め，sin を使ってスケール係数を計算
        float theta = std::acos(dot);
        float sinTheta = std::sin(theta);

        // 正規ケース：scale0, scale1 を計算して補間
        float scale0 = std::sin((1.0f - t) * theta) / sinTheta;
        float scale1 = std::sin(t * theta) / sinTheta;

        Quaternion result{};
        result.x = scale0 * q0_.x + scale1 * q1.x;
        result.y = scale0 * q0_.y + scale1 * q1.y;
        result.z = scale0 * q0_.z + scale1 * q1.z;
        result.w = scale0 * q0_.w + scale1 * q1.w;

        return result;
    }

    // Quaternionからオイラー角へ変換
    Vector3 ToEuler(const Quaternion& q) {
        Matrix4x4 rotMat = MakeRotateMatrix(q);
        return ExtractEulerFromMatrix(rotMat);
    }

#pragma endregion

#pragma region 衝突判定

    // 球と球の衝突判定
    bool IsCollision(const Vector3& s1_center, const float& s1_radius, const Vector3& s2_center, const float& s2_radius) {

        // 2つの球の中心点間の距離を求める
        float distance = Length(Subtract(s2_center, s1_center));
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
        float distance = Length(Subtract(s2.center, s1.center));
        // 半径の合計よりも短ければ衝突
        if (distance <= s1.radius + s2.radius) {
            // 当たった処理を諸々
            return true;
        }

        return false;
    }

    // 平面と球の衝突判定
    bool IsCollision(const Sphere& sphere, const Plane& plane) {
        float k = std::fabs(Dot(plane.normal, sphere.center) - plane.distance);
        if (k <= sphere.radius) {
            return true;
        }
        return false;
    }

    // 線分と平面の衝突判定
    bool IsCollision(const Segment& segment, const Plane& plane) {

        // まずは垂直判定を行うために、法線と線の内積を求める
        float dot = Dot(plane.normal, segment.diff);

        // 垂直 = 平行であるので、衝突しているはずがない
        if (dot == 0.0f) {
            return false;
        }

        // tを求める
        float t = (plane.distance - Dot(segment.origin, plane.normal)) / dot;

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
        float dot = Dot(plane.normal, ray.diff);

        // 垂直 = 平行であるので、衝突しているはずがない
        if (dot == 0.0f) {
            return false;
        }

        // tを求める
        float t = (plane.distance - Dot(ray.origin, plane.normal)) / dot;

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
        float dot = Dot(plane.normal, line.diff);

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
        Vector3 normal = Cross(Subtract(triangle.vertices_[1], triangle.vertices_[0]), Subtract(triangle.vertices_[2], triangle.vertices_[0]));

        // 平面と線分の内積（垂直＝平行チェック）
        float dot = Dot(normal, segment.diff);
        if (dot == 0.0f) {
            return false; // 平行なので交差しない
        }

        // 平面と線分の交点を求める
        float t = Dot(normal, Subtract(triangle.vertices_[0], segment.origin)) / dot;
        // t が [0,1] にないなら線分上に交点がない
        if (t < 0.0f || t > 1.0f) {
            return false;
        }

        // 交点を求める
        Vector3 p = Add(segment.origin, Multiply(t, segment.diff));

        // 各辺を結んだベクトルと、頂点と衝突点pを結んだベクトルのクロス積を取る
        Vector3 v01 = Subtract(triangle.vertices_[1], triangle.vertices_[0]);
        Vector3 v1p = Subtract(p, triangle.vertices_[1]);
        Vector3 cross01 = Cross(v01, v1p);
        Vector3 v12 = Subtract(triangle.vertices_[2], triangle.vertices_[1]);
        Vector3 v2p = Subtract(p, triangle.vertices_[2]);
        Vector3 cross12 = Cross(v12, v2p);
        Vector3 v20 = Subtract(triangle.vertices_[0], triangle.vertices_[2]);
        Vector3 v0p = Subtract(p, triangle.vertices_[0]);
        Vector3 cross20 = Cross(v20, v0p);
        // すべての小三角形のクロス積と法線が同じ方向を向いていたら衝突
        if (Dot(cross01, normal) >= 0.0f && Dot(cross12, normal) >= 0.0f && Dot(cross20, normal) >= 0.0f) {
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
        float distance = Length(Subtract(closestPoint, sphere.center));
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

#pragma endregion

    Vector3 Perpendicular(const Vector3& vector) {
        if (vector.x != 0.0f || vector.y != 0.0f) {
            return { -vector.y, vector.x, 0.0f };
        }
        return { 0.0f, -vector.z, vector.y };
    }
}