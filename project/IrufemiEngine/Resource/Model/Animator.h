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
    void Play(const std::string& filename, bool loop = true, float fadeDuration = 0.0f);
    
    /// @brief 時間を進めて指定されたポーズを更新する
    void Update(SkeletonPose& targetPose);

    /// @brief ルートモーション用の移動量（1フレーム間の差分）を取得する
    Vector3 GetDeltaRootTranslation() const { return deltaRootTranslation_; }
    Quaternion GetDeltaRootRotation() const { return deltaRootRotation_; }

    /// @brief デバッグ用UI
    void Debug(const char* objName = " ");

    void SetPlaybackSpeed(float speed) { playbackSpeed_ = speed; }
    float GetPlaybackSpeed() const { return playbackSpeed_; }

    const Animation* GetCurrentAnimation() const { return currentAnimation_.get(); }

private:
    IrufemiEngine* engine_ = nullptr;
    
    // 現在のアニメーション
    std::shared_ptr<Animation> currentAnimation_;
    std::string currentAnimationName_;
    float animationTime_ = 0.0f;
    float playbackSpeed_ = 1.0f;
    bool isLooping_ = true;

    // ブレンド（クロスフェード）用
    std::shared_ptr<Animation> previousAnimation_;
    std::string previousAnimationName_;
    float previousAnimationTime_ = 0.0f;
    float fadeTimer_ = 0.0f;
    float fadeDuration_ = 0.0f;
    bool isBlending_ = false;

    // ルートモーション用
    Vector3 deltaRootTranslation_ = {0.0f, 0.0f, 0.0f};
    Quaternion deltaRootRotation_ = {0.0f, 0.0f, 0.0f, 1.0f};

    // ヘルパ
    void ExtractRootMotion(const Animation* anim, const SkeletonData* skeleton, float prevTime, float currTime, Vector3& outDeltaTrans, Quaternion& outDeltaRot);
};
