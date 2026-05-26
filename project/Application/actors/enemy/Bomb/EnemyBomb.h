#pragma once
#include "Irufemi.h"
#include "core/math/Transform.h"
#include "core/math/geometry/OBB.h"
#include <memory>
#include <vector>
#include <wrl.h>
#include "IrufemiEngine/Renderer/ParticleGPU/GPUParticleSystem.h"
#include "IrufemiEngine/Engine/Graphics/Data/ExplosionParams.h"
#include "IrufemiEngine/Engine/Graphics/Data/BombCoreParams.h"
#include "IrufemiEngine/Renderer/Object3D/Primitive/PrimitiveObjects3DClass.h"
#include "IrufemiEngine/Renderer/Object3D/Primitive/SphereClass.h"

class Camera;
class IrufemiEngine;

class EnemyBomb {
public:
    ~EnemyBomb();
    void Initialize(IrufemiEngine* engine);
    
    /**
     * @brief 爆弾を投げる
     * @param startPos 投擲開始位置
     * @param targetPos 目標位置（着弾点）
     */
    void Throw(const Vector3& startPos, const Vector3& targetPos);

    void Update();
    void Draw(IrufemiEngine* engine);

    bool IsExpired() const { return isExpired_; }
    bool IsExploding() const { return state_ == State::Exploding; }
    void Cancel();

    std::vector<OBB> GetOBBs() const;

private:
    enum class State {
        Idle,
        Flying,
        Telegraphing,
        Exploding,
        Done
    };

    State state_ = State::Idle;

    // 爆弾本体（飛行中）
    std::shared_ptr<PrimitiveObjects3DClass> bombSphere_ = nullptr;
    Transform bombTransform_;
    
    // 飛行用パラメータ
    Vector3 startPos_ = {};
    Vector3 targetPos_ = {};
    float flightTimer_ = 0.0f;
    float flightDuration_ = 1.0f; // 飛行にかかる時間
    float throwHeight_ = 10.0f;   // 放物線の高さ

    // 十字爆発の予告用（X軸・Z軸に沿った2つのボックス）
    std::shared_ptr<PrimitiveObjects3DClass> telegraphObjX_ = nullptr;
    std::shared_ptr<PrimitiveObjects3DClass> telegraphObjZ_ = nullptr;
    // 十字爆発の攻撃用
    std::shared_ptr<PrimitiveObjects3DClass> attackCylinderX_ = nullptr;
    std::shared_ptr<PrimitiveObjects3DClass> attackCylinderZ_ = nullptr;
    Transform attackTransformX_;
    Transform attackTransformZ_;

    // 爆発用パラメータ
    float telegraphTimer_ = 0.0f;
    float telegraphDuration_ = 5.0f; // 予告時間
    float telegraphBlinkThreshold_ = 0.7f; // 点滅開始タイミング
    float telegraphBlinkFrequency_ = 30.0f; // 高速点滅周波数
    float explodeTimer_ = 0.0f;
    float explodeDuration_ = 0.5f;   // 爆発の持続時間
    float explosionLength_ = 200.0f;  // 爆発の長さ
    float explosionThickness_ = 10.0f; // 爆発の太さ
    float telegraphHeight_ = 0.5f;    // 予告線の高さ

    // エフェクト用
    Microsoft::WRL::ComPtr<ID3D12Resource> explosionParamsResource_;
    ExplosionParams* explosionParamsData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> bombCoreParamsResource_;
    BombCoreParams* bombCoreParamsData_ = nullptr;
    std::unique_ptr<GPUParticleSystem> gpuParticle_ = nullptr;

    bool isExpired_ = false;
    IrufemiEngine* engine_ = nullptr;
};
