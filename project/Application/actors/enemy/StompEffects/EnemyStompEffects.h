#pragma once
#include "core/math/Transform.h"
#include "core/math/Vector4.h"
#include "core/math/geometry/OBB.h"
#include "Engine/Core/Shape/Sphere.h"
#include <memory>
#include <algorithm>
#include <wrl.h>
#include <d3d12.h>
#include "Renderer/ParticleGPU/GPUParticleSystem.h"

// 前方宣言
class Camera;
class ObjClass;
class PrimitiveObjects3DClass;

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
        int explosionDamage = 50;

        // 2. リング（予兆範囲）
        Vector3 ringScale = { 1.2f, 0.5f, 1.2f };
        float ringExpandDuration = 5.0f;
        float ringMaxRadius = 160.0f;
        float ringThickness = 2.5f;
        float ringHeight = 0.01f;      
        float ringGroundOffset = -2.6f;
        Vector4 ringColorNormal = { 1.0f, 0.0f, 0.0f, 0.5f };
        Vector4 ringColorWarning = { 1.0f, 1.0f, 0.0f, 1.0f };
        float ringKeepDuration = 2.0f;

        // 3. 再爆発（噴き上がり）
        float finalExplosionDuration = 1.0f;
        float finalExplosionMaxHeight = 30.0f;  // 上に伸びる高さ
        float finalExplosionMaxRadius = 170.0f;  // 噴き出す横幅
        int finalExplosionDamage = 50;
    };

    void Initialize(class IrufemiEngine* engine);
    void Update(float deltaTime);
    void Draw(class IrufemiEngine* engine);
    void Fire(const Vector3& position);
    void Cancel(); // 強制キャンセル用
    bool IsActive() const { return isActive_; }

    // 死亡演出専用：即座にド派手な大爆発（炎パーティクル15,000発）を発生させる
    void FireDeathExplosion(const Vector3& position);

    /// @brief デバッグ描画（当たり判定等の可視化用）
    void DrawDebug(class Line3DRegion* lineRegion);

    void SetCamera(Camera* camera) { camera_ = camera; }
    void SetParameters(const Parameters& params) { params_ = params; }

    // --- 本体落下予兆（AOE） ---
    void StartBodyTelegraph(const Vector3& pos, float radius);
    void UpdateBodyTelegraph(const Vector3& pos, float warningRatio);
    void StopBodyTelegraph();
    void DrawBodyTelegraph(class IrufemiEngine* engine);

    // --- 当たり判定用ゲッター ---
    bool IsExplosionDamageActive() const;
    float GetExplosionRadius() const;
    const Vector3& GetBasePosition() const { return basePosition_; }
    int GetExplosionDamage() const { return params_.explosionDamage; }
    
    bool IsFinalExplosionActive() const { return currentPhase_ == Phase::FinalExplosion; }
    const Sphere& GetFinalExplosionSphere() const { return finalExplosionSphere_; }
    int GetFinalExplosionDamage() const { return params_.finalExplosionDamage; }

    bool HasDealtExplosionDamage() const { return hasDealtExplosionDamage_; }
    void SetDealtExplosionDamage(bool dealt) { hasDealtExplosionDamage_ = dealt; }
    
    bool HasDealtFinalDamage() const { return hasDealtFinalDamage_; }
    void SetDealtFinalDamage(bool dealt) { hasDealtFinalDamage_ = dealt; }

private:
    // 判定用ヘルパー
    void UpdateFinalExplosionSphere();

    Camera* camera_ = nullptr;
    std::unique_ptr<PrimitiveObjects3DClass> explosionObj_ = nullptr;
    std::unique_ptr<PrimitiveObjects3DClass> ringObj_ = nullptr;
    std::unique_ptr<PrimitiveObjects3DClass> bodyTelegraphObj_ = nullptr; // 落下地点予兆用AOE
    std::unique_ptr<GPUParticleSystem> gpuParticleSystem_ = nullptr; // 大爆発の火の粉用

    Transform explosionTransform_;
    Transform ringTransform_;
    Transform bodyTelegraphTransform_;

    bool isActive_ = false;
    bool isBodyTelegraphActive_ = false;
    Phase currentPhase_ = Phase::Expanding;
    float globalTimer_ = 0.0f; // 最初の爆発用
    float phaseTimer_ = 0.0f;  // リング・再爆発フェーズ用
    Vector3 basePosition_ = {};

    Parameters params_;
    Sphere finalExplosionSphere_;

    bool hasDealtExplosionDamage_ = false;
    bool hasDealtRingDamage_ = false;
    bool hasDealtFinalDamage_ = false;

    float Lerp(float start, float end, float t) const { return start + (end - start) * t; }
};