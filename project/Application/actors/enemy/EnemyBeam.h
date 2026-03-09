#pragma once
#include "Irufemi.h"
#include "core/math/Transform.h"
#include "core/math/geometry/OBB.h"
#include <memory>

class Camera;

class EnemyBeam {
public:
    void Initialize(Camera* camera, const Matrix4x4& muzzleMatrix);
    void Update(const Matrix4x4& muzzleMatrix);
    void Draw();

    OBB GetOBB() const;
    bool IsExpired() const { return isExpired_; }

    // 可視・不可視の切り替え用
    void SetVisible(bool visible) { isVisible_ = visible; }

private:
    std::unique_ptr<ObjClass> obj_ = nullptr;
    Transform transform_;

    // --- 判定用パラメータ ---
    float beamLength_ = 100.0f;    // 固定の長さ
    float beamThickness_ = 10.0f;  // 当たり判定の太さ
    float lifeTime_ = 1.0f;       // 攻撃持続時間
    float timer_ = 0.0f;

    bool isExpired_ = false;
    bool isVisible_ = true;       // falseにすれば描画されなくなる

    float forwardOffset_ = 2.0f;
};