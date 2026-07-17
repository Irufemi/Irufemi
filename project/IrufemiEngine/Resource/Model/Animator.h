#pragma once

#include <string>
#include <memory>
#include "Resource/Model/Data/Animation.h"
#include "Resource/Model/Data/SkeletonPose.h"

class IrufemiEngine;

/**
 * @class Animator
 * @brief GameObjectに依存せず、アニメーションの再生とポーズの計算を行う純粋なロジッククラス。
 * AnimatedMeshObject 等に適用するポーズ（SkeletonPose）を更新します。
 */
class Animator {
public:
    Animator();
    ~Animator();

    void Initialize(IrufemiEngine* engine);
    
    /// @brief アニメーションをロードして再生する
    void Play(const std::string& filename, bool loop = true);
    
    /// @brief 時間を進めて指定されたポーズを更新する
    void Update(SkeletonPose& targetPose);

    /// @brief デバッグ用UI
    void Debug(const char* objName = " ");

    void SetPlaybackSpeed(float speed) { playbackSpeed_ = speed; }
    float GetPlaybackSpeed() const { return playbackSpeed_; }

    const Animation* GetCurrentAnimation() const { return currentAnimation_.get(); }

private:
    IrufemiEngine* engine_ = nullptr;
    std::shared_ptr<Animation> currentAnimation_;
    std::string currentAnimationName_;
    float animationTime_ = 0.0f;
    float playbackSpeed_ = 1.0f;
    bool isLooping_ = true;
};
