#pragma once
#include "core/math/Transform.h"
#include <memory>

// 前方宣言
class Camera;
class Player;
class ObjClass; // あなたのプロジェクトでのモデルクラス

class EnemyStompEffects {
public:
    void Initialize(Camera* camera);
    void Update(float deltaTime, Player* player);
    void Draw();

    void Fire(const Vector3& position);

    bool IsActive() const { return isActive_; }

private:
    Camera* camera_ = nullptr;
    // EnemyBeamに合わせて ObjClass を使用
    std::unique_ptr<ObjClass> explosionObj_ = nullptr;
    std::unique_ptr<ObjClass> ringObj_ = nullptr;

    Transform explosionTransform_;
    Transform ringTransform_;

    bool isActive_ = false;
    float timer_ = 0.0f;
    Vector3 basePosition_;

    // パラメータ
    float explosionDuration_ = 0.5f;
    float explosionRadius_ = 8.0f;
    float ringDuration_ = 1.5f;
    float ringMaxRadius_ = 35.0f;
    float ringThickness_ = 1.5f;

    bool hasDealtExplosionDamage_ = false;
    bool hasDealtRingDamage_ = false;
};