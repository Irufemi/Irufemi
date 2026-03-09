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

    // 当たり判定用
    OBB GetOBB() const;
    bool IsExpired() const { return isExpired_; }

private:
    std::unique_ptr<ObjClass> obj_ = nullptr;
    Transform transform_;

    // --- 調整用パラメータ ---
    float maxLength_ = 50.0f;     // ビームの最大到達距離
    float beamThickness_ = 0.5f;  // ビームの太さ
    float lifeTime_ = 2.0f;       // ビームの生存時間（秒）
    float timer_ = 0.0f;

    bool isExpired_ = false;      // 消滅フラグ
};