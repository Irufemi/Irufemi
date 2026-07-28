#pragma once

#include "Framework/Component/Component.h"
#include "Resource/Model/Animator.h"
#include <memory>
#include <string>

/**
 * @class AnimatorComponent
 * @brief エディタ対応のアニメーション制御コンポーネント。
 * 同じ GameObject 内の SkinnedMeshRendererComponent を探し、ポーズを流し込みます。
 */
class AnimatorComponent : public Component {
public:
    AnimatorComponent();
    ~AnimatorComponent() override;

    void Initialize() override;
    void Start() override;
    void Update() override;
    
    void OnRegisterProperties() override;

    void Play(const std::string& animationName, bool loop = true, float fadeDuration = 0.0f);
    
    std::string GetComponentName() const override { return "AnimatorComponent"; }
    
    Animator* GetRawAnimator() { return animator_.get(); }

private:
    std::unique_ptr<Animator> animator_;
    std::string defaultAnimation_ = "";
    std::string currentLoadedAnimation_ = "";
    float playbackSpeed_ = 1.0f;
    bool applyRootMotion_ = false;
};
