#include "Framework/Component/Logic/AnimatorComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Framework/Component/Renderer/SkinnedMeshRendererComponent.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"

AnimatorComponent::AnimatorComponent() {
    animator_ = std::make_unique<Animator>();
}

AnimatorComponent::~AnimatorComponent() {}

void AnimatorComponent::Initialize() {
    if (GetGameObject() && GetGameObject()->GetScene()) {
        animator_->Initialize(GetGameObject()->GetScene()->GetEngine());
    }
}

void AnimatorComponent::Start() {
    if (GetGameObject() && GetGameObject()->GetScene()) {
        animator_->Initialize(GetGameObject()->GetScene()->GetEngine());
    }

    if (!defaultAnimation_.empty()) {
        Play(defaultAnimation_, true);
    }
}

void AnimatorComponent::Play(const std::string& animationName, bool loop, float fadeDuration) {
    defaultAnimation_ = animationName;
    currentLoadedAnimation_ = animationName;
    animator_->Play(animationName, loop, fadeDuration);
}

void AnimatorComponent::Update() {
    // エディタ等で文字列が変更された場合の動的ロード検知
    if (defaultAnimation_ != currentLoadedAnimation_) {
        Play(defaultAnimation_, true); // デフォルトループで再ロード
    }

    animator_->SetPlaybackSpeed(playbackSpeed_);

    // 同じGameObjectについている SkinnedMeshRenderer を探す
    auto renderer = GetGameObject()->GetComponent<SkinnedMeshRendererComponent>();
    if (renderer && renderer->GetRawObject()) {
        SkeletonPose* pose = renderer->GetRawObject()->GetInternalSkeletonPose();
        if (pose && pose->data) {
            animator_->Update(*pose);

            // ルートモーションをGameObjectのTransformに適用する
            if (applyRootMotion_ && GetGameObject()) {
                auto transform = GetTransform();
                if (transform) {
                    Irufemi::Vector3 deltaTrans = animator_->GetDeltaRootTranslation();
                    Irufemi::Quaternion deltaRot = animator_->GetDeltaRootRotation();

                    // 現在のGameObjectのTransformに対して適用
                    // TODO: 厳密にはキャラクターの現在の回転を考慮してdeltaTransをワールド空間に変換して足す必要がある
                    // 今回は簡易的にローカル移動量をそのまま加算する
                    transform->SetPosition(Irufemi::Math::Add(transform->GetPosition(), deltaTrans));

                    // 回転も合成する場合（TransformComponentがオイラー角ならQuaternionに変換してから合成して戻す）
                    // ひとまず移動のみ適用でも効果は確認できる
                }
            }

            // SkinnedMeshRenderer に対して計算済みのポーズを描画に使うよう指示
            renderer->SetPoseOverride(pose);
        }
    }
}

void AnimatorComponent::OnRegisterProperties() {
    RegisterProperty("Default Animation", &defaultAnimation_).SetTooltip("The name of the initial animation to play");

    RegisterProperty("Playback Speed", &playbackSpeed_).SetTooltip("Animation playback speed multiplier");

    RegisterProperty("Apply Root Motion", &applyRootMotion_)
        .SetTooltip("Apply animation root motion to GameObject transform");
}
