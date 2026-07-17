#include "Animator.h"
#include "Engine/IrufemiEngine.h"
#include "Resource/Model/AnimationManager.h"
#include "Engine/Manager/DebugUI.h"
#include <cmath>

#if defined USE_IMGUI
#include <imgui.h>
#endif

Animator::Animator() {}
Animator::~Animator() {}

void Animator::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
}

void Animator::Play(const std::string& filename, bool loop) {
    if (!engine_ || !engine_->GetAnimationManager()) return;

    currentAnimation_ = engine_->GetAnimationManager()->LoadAnimationFile(filename);
    currentAnimationName_ = filename;
    animationTime_ = 0.0f;
    isLooping_ = loop;
}

void Animator::Update(SkeletonPose& targetPose) {
    if (!currentAnimation_ || !engine_) return;

    // 時間を進める
    animationTime_ += engine_->GetGameDeltaTime() * playbackSpeed_;
    
    if (currentAnimation_->duration > 0.0f) {
        if (isLooping_) {
            animationTime_ = std::fmod(animationTime_, currentAnimation_->duration);
        } else {
            animationTime_ = (std::min)(animationTime_, currentAnimation_->duration);
        }
    }

    // ポーズにアニメーションを適用
    AnimationManager::ApplyAnimation(targetPose, *currentAnimation_, animationTime_);
    AnimationManager::SkeletonUpdate(targetPose);
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
        ImGui::TreePop();
    }
#endif
}
