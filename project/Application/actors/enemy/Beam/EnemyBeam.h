#pragma once
#include "Irufemi.h"
#include "core/math/Transform.h"
#include "core/math/geometry/OBB.h"
#include <memory>

class Camera;

class EnemyBeam {
public:
    // 初期化：muzzleMatrixは発射口（頭部）のワールド行列
    void Initialize(Camera* camera, const Matrix4x4& muzzleMatrix);

    // 更新：毎フレーム頭の位置に合わせる
    void Update(const Vector3& headPos, const Vector3& playerPos);

    // 描画
    void Draw();

    // --- 当たり判定・状態管理 ---
    OBB GetOBB() const;
    bool IsExpired() const { return isExpired_; }
    void SetActive(bool active) { isActive_ = active; }
    bool IsActive() const { return isActive_; }

    // --- 外見変更用の追加機能 ---
    // ビームの太さを変更する
    void SetThickness(float thickness) {
        beamThickness_ = thickness;
        transform_.scale = { beamThickness_, beamThickness_, beamLength_ };
    }
    // ビームの色を変更する（RGBA）
    void SetColor(const Vector4& color) {
        if (obj_) obj_->SetColor(color);
    }

private:
    std::unique_ptr<ObjClass> obj_ = nullptr;
    Transform transform_;

    float beamLength_ = 100.0f;    // 固定の長さ
    float beamThickness_ = 1.0f;   // 現在の太さ
    float lifeTime_ = 1.5f;        // 攻撃持続時間（発射開始からの時間）
    float timer_ = 0.0f;

    bool isExpired_ = false;
    bool isActive_ = false;        // 発射中フラグ
    float forwardOffset_ = 0.0f;   // 中心位置調整用
};