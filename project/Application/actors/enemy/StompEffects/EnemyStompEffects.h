#pragma once
#include "core/math/Transform.h"
#include "core/math/Vector4.h"
#include "core/math/geometry/OBB.h"
#include <memory>
#include <algorithm>

// 前方宣言
class Camera;
class Player;
class ObjClass;

class EnemyStompEffects {
public:
    // --- 状態定義 ---
    enum class Phase {
        Expanding,      // 1. リング拡大
        KeepAndWarning, // 2. 最大サイズで維持 ＋ 警告（赤らむ）
        FinalExplosion, // 3. 下から上へ噴き上がる爆発
        Finished        // 4. 終了
    };

    // --- 全てのパラメータをここに集約 ---
    struct Parameters {
        // 1. 最初の足元爆発
        float explosionDuration = 2.0f;
        float explosionMaxRadius = 40.0f;
        float explosionInitialAlpha = 0.8f;
        float explosionDamageActiveTime = 0.3f;

        // 2. リング（予兆範囲）
        Vector3 ringScale = { 1.2f, 0.5f, 1.2f };
        float ringExpandDuration = 5.0f;
        float ringMaxRadius = 160.0f;
        float ringThickness = 2.5f;
        float ringHeight = 0.01f;      // ★復活させました
        float ringGroundOffset = -2.6f;
        Vector4 ringColorNormal = { 1.0f, 0.0f, 0.0f, 0.5f };
        Vector4 ringColorWarning = { 1.0f, 1.0f, 0.0f, 1.0f };
        float ringKeepDuration = 2.0f;

        // 3. 再爆発（噴き上がり）
        float finalExplosionDuration = 1.0f;
        float finalExplosionMaxHeight = 30.0f;  // 上に伸びる高さ
        float finalExplosionMaxRadius = 165.0f;  // 噴き出す横幅
    };

    void Initialize(Camera* camera);
    void Update(float deltaTime, Player* player);
    void Draw();
    void Fire(const Vector3& position);
    bool IsActive() const { return isActive_; }
    void SetParameters(const Parameters& params) { params_ = params; }

private:
    // 判定用ヘルパー
    void UpdateFinalExplosionOBB();
    bool CheckOBBCollision(const OBB& obb, const Vector3& point);

    Camera* camera_ = nullptr;
    std::unique_ptr<ObjClass> explosionObj_ = nullptr;
    std::unique_ptr<ObjClass> ringObj_ = nullptr;
    std::unique_ptr<ObjClass> finalExplosionObj_ = nullptr; // 噴き上がり用モデル

    Transform explosionTransform_;
    Transform ringTransform_;
    Transform finalExplosionTransform_;

    bool isActive_ = false;
    Phase currentPhase_ = Phase::Expanding;
    float globalTimer_ = 0.0f; // 最初の爆発用
    float phaseTimer_ = 0.0f;  // リング・再爆発フェーズ用
    Vector3 basePosition_;

    Parameters params_;
    OBB finalExplosionOBB_;

    bool hasDealtExplosionDamage_ = false;
    bool hasDealtRingDamage_ = false;
    bool hasDealtFinalDamage_ = false;

    float Lerp(float start, float end, float t) { return start + (end - start) * t; }
};