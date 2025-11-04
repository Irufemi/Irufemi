#include "ScreenSpace.h"

#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Matrix4x4.h"
#include "camera/Camera.h"
#include "function/Math.h"
#include <cmath>

Vector3 ScreenToWorldOnZ(const Camera* cam, const Vector2& screen, float targetZ) {
    // 既存コードと同じ積順・APIを使用
    Matrix4x4 view = cam->GetViewMatrix();
    Matrix4x4 proj = cam->GetPerspectiveFovMatrix();
    Matrix4x4 vp = cam->GetViewportMatrix();
    Matrix4x4 vpv = Math::Multiply(view, Math::Multiply(proj, vp));
    Matrix4x4 inv = Math::Inverse(vpv);

    Vector3 p0 = Math::Transform(Vector3{ screen.x, screen.y, 0.0f }, inv);
    Vector3 p1 = Math::Transform(Vector3{ screen.x, screen.y, 1.0f }, inv);
    Vector3 dir = Math::Subtract(p1, p0);

    float denom = dir.z;
    if (std::fabs(denom) < 1e-6f) {
        return p0; // 退避（視線がZ平面と平行）
    }
    float t = (targetZ - p0.z) / denom;
    return Math::Add(p0, Math::Multiply(t, dir));
}

// 画面上の半径[pixels]を、Z=targetZ 平面上のワールド半径へ変換
float ScreenRadiusToWorld(const Camera* cam, const Vector2& center, float radiusPx, float targetZ) {
    Vector3 wc = ScreenToWorldOnZ(cam, center, targetZ);
    Vector3 wx = ScreenToWorldOnZ(cam, Vector2{ center.x + radiusPx, center.y }, targetZ);
    Vector2 d = Math::Subtract(Vector2{ wx.x, wx.y }, Vector2{ wc.x, wc.y });
    return Math::Length(d);
}