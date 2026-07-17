#include "AnimatorComponent.h"
#include "../../GameObject.h"
#include "../Renderer/SkinnedMeshRendererComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Framework/BaseScene.h"

AnimatorComponent::AnimatorComponent() {
    animator_ = std::make_unique<Animator>();
}

AnimatorComponent::~AnimatorComponent() {}

void AnimatorComponent::Initialize() {
    if (GetGameObject() && GetGameObject()->GetScene()) {
        animator_->Initialize(GetGameObject()->GetScene()->GetEngine());
    }
    if (!defaultAnimation_.empty()) {
        Play(defaultAnimation_, true);
    }
}

void AnimatorComponent::Play(const std::string& animationName, bool loop) {
    defaultAnimation_ = animationName;
    animator_->Play(animationName, loop);
}

void AnimatorComponent::Update() {
    animator_->SetPlaybackSpeed(playbackSpeed_);

    // 同じGameObjectについている SkinnedMeshRenderer を探す
    auto renderer = GetGameObject()->GetComponent<SkinnedMeshRendererComponent>();
    if (renderer && renderer->GetRawObject()) {
        SkeletonPose* pose = renderer->GetRawObject()->GetInternalSkeletonPose();
        if (pose && pose->data) {
            animator_->Update(*pose);
            // SkinnedMeshRenderer に対して計算済みのポーズを描画に使うよう指示
            renderer->GetRawObject()->Update(pose);
        }
    }
}

void AnimatorComponent::OnRegisterProperties() {
    RegisterProperty("Default Animation", &defaultAnimation_)
        .SetTooltip("The name of the initial animation to play");
        
    RegisterProperty("Playback Speed", &playbackSpeed_)
        .SetTooltip("Animation playback speed multiplier");
}
