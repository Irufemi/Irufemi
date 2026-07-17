#include "Animator.h"
#include "Engine/IrufemiEngine.h"
#include "Resource/Model/AnimationManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Core/Math/Math.h"
#include <cmath>

#if defined USE_IMGUI
#include <imgui.h>
#endif

Animator::Animator() {}
Animator::~Animator() {}

void Animator::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
}

void Animator::Play(const std::string& filename, bool loop, float fadeDuration) {
    if (!engine_ || !engine_->GetAnimationManager()) return;

    auto newAnim = engine_->GetAnimationManager()->LoadAnimationFile(filename);
    if (!newAnim) return;

    if (currentAnimation_ && fadeDuration > 0.0f && currentAnimationName_ != filename) {
        previousAnimation_ = currentAnimation_;
        previousAnimationName_ = currentAnimationName_;
        previousAnimationTime_ = animationTime_;
        fadeTimer_ = 0.0f;
        fadeDuration_ = fadeDuration;
        isBlending_ = true;
    } else {
        isBlending_ = false;
        previousAnimation_.reset();
    }

    currentAnimation_ = newAnim;
    currentAnimationName_ = filename;
    animationTime_ = 0.0f;
    isLooping_ = loop;
    deltaRootTranslation_ = {0.0f, 0.0f, 0.0f};
    deltaRootRotation_ = {0.0f, 0.0f, 0.0f, 1.0f};
}

void Animator::Update(SkeletonPose& targetPose) {
    if (!currentAnimation_ || !engine_) return;

    float deltaTime = engine_->GetGameDeltaTime() * playbackSpeed_;
    float prevTime = animationTime_;
    animationTime_ += deltaTime;
    
    if (currentAnimation_->duration > 0.0f) {
        if (isLooping_) {
            animationTime_ = std::fmod(animationTime_, currentAnimation_->duration);
            // ループをまたいだ時のルートモーション処理が必要な場合はここで行う（簡易実装では省略）
            if (animationTime_ < prevTime) {
                prevTime = 0.0f; // ループの瞬間は0からの差分とする等
            }
        } else {
            animationTime_ = (std::min)(animationTime_, currentAnimation_->duration);
        }
    }

    // Root Motion の抽出
    ExtractRootMotion(currentAnimation_.get(), prevTime, animationTime_, deltaRootTranslation_, deltaRootRotation_);

    if (isBlending_ && previousAnimation_) {
        fadeTimer_ += engine_->GetGameDeltaTime(); // フェードは等速(playbackSpeedに依存しない)
        float prevDeltaTime = engine_->GetGameDeltaTime() * playbackSpeed_; // previousAnimの速度は一旦同じにする
        previousAnimationTime_ += prevDeltaTime;
        if (previousAnimation_->duration > 0.0f && isLooping_) {
            previousAnimationTime_ = std::fmod(previousAnimationTime_, previousAnimation_->duration);
        }

        float weight = fadeTimer_ / fadeDuration_;
        if (weight >= 1.0f) {
            isBlending_ = false;
            previousAnimation_.reset();
            AnimationManager::ApplyAnimation(targetPose, *currentAnimation_, animationTime_, false); // Rootは適用しない(外部で移動)
        } else {
            // ブレンド適用
            AnimationManager::BlendAnimation(targetPose, *previousAnimation_, previousAnimationTime_, *currentAnimation_, animationTime_, weight, false);
        }
    } else {
        // 通常の適用
        AnimationManager::ApplyAnimation(targetPose, *currentAnimation_, animationTime_, false);
    }
    
    AnimationManager::SkeletonUpdate(targetPose);
}

void Animator::ExtractRootMotion(const Animation* anim, float prevTime, float currTime, Vector3& outDeltaTrans, Quaternion& outDeltaRot) {
    outDeltaTrans = {0.0f, 0.0f, 0.0f};
    outDeltaRot = {0.0f, 0.0f, 0.0f, 1.0f};
    if (!anim) return;

    // Rootボーンの判定。とりあえず最初に見つかったアニメーションをRootとみなすか、"Root"等の名前で探す
    // ここでは、データ構造上どれがRootボーンかSkeletonPose無しでは分からないため、
    // "RootNode" のような一般的な名前を探すか、最初のノードを使う。
    // ※今回は簡易的に一番最初のノードアニメーションの差分をRootMotionとする
    if (anim->nodeAnimations.empty()) return;
    
    // 仮: iterの最初をRootとする (あるいは特定のボーン名)
    auto rootIt = anim->nodeAnimations.begin();
    const NodeAnimation& rootAnim = rootIt->second;

    if (!rootAnim.translate.keyframes.empty()) {
        Vector3 prevPos = AnimationManager::CalculateValue(rootAnim.translate, prevTime);
        Vector3 currPos = AnimationManager::CalculateValue(rootAnim.translate, currTime);
        outDeltaTrans = currPos - prevPos;
    }
    if (!rootAnim.rotate.keyframes.empty()) {
        Quaternion prevRot = AnimationManager::CalculateValue(rootAnim.rotate, prevTime);
        Quaternion currRot = AnimationManager::CalculateValue(rootAnim.rotate, currTime);
        // Quaternionの差分 (prevRot^-1 * currRot)
        outDeltaRot = Math::Multiply(Math::Inverse(prevRot), currRot);
    }
}

void Animator::Debug([[maybe_unused]] const char* objName) {
#if defined USE_IMGUI
    std::string name = std::string("Animator: ") + objName;
    if (ImGui::TreeNode(name.c_str())) {
        ImGui::Text("Current Animation: %s", currentAnimationName_.c_str());
        ImGui::SliderFloat("Playback Speed", &playbackSpeed_, 0.0f, 3.0f);
        ImGui::Checkbox("Loop", &isLooping_);
        if (currentAnimation_) {
            engine_->GetDebugUI()->DebugAnimationControl(*currentAnimation_, animationTime_);
        }
        if (ImGui::Button("Reset Time")) {
            animationTime_ = 0.0f;
        }
        if (isBlending_) {
            ImGui::ProgressBar(fadeTimer_ / fadeDuration_, ImVec2(0.0f, 0.0f), "Blending...");
        }
        ImGui::TreePop();
    }
#endif
}
