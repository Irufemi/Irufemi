#pragma once

#include <memory>
#include "Engine/Core/Math/Vector3.h"

// 前方宣言
class GPUParticleSystem;
class IrufemiEngine;

enum class FireworkState {
    Waiting,
    Ascending,
    Bursting,
    Finished
};

/**
 * @class FireworkEffect
 * @brief C++で弾道を管理し、GPUパーティクルで花火の火の粉・破裂を描画する専用クラス
 */
class FireworkEffect {
public:
    FireworkEffect();
    ~FireworkEffect();

    void Initialize(IrufemiEngine* engine);
    void Update(float deltaTime);
    void Draw();

    /**
     * @brief 打ち上げを開始する
     * @param startPos 打ち上げ開始位置
     * @param targetPos 爆発する目標位置
     */
    void Fire(const Vector3& startPos, const Vector3& targetPos);

    bool IsFinished() const { return state_ == FireworkState::Finished; }
    bool IsWaiting() const { return state_ == FireworkState::Waiting; }
    void Reset() { state_ = FireworkState::Waiting; }

private:
    std::unique_ptr<GPUParticleSystem> trailParticles_;
    std::unique_ptr<GPUParticleSystem> burstParticles_;

    FireworkState state_ = FireworkState::Waiting;
    
    Vector3 currentPos_;
    Vector3 targetPos_;
    Vector3 velocity_;

    float burstTimer_ = 0.0f;
};
