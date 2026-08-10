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

    /**
     * @brief Initialize を実行する。
     */
    void Initialize() override;
    /**
     * @brief Start を実行する。
     */
    void Start() override;
    /**
     * @brief Update を実行する。
     */
    void Update() override;
    
    /**
     * @brief OnRegisterProperties を実行する。
     */
    void OnRegisterProperties() override;

    /**
     * @brief Play を実行する。
     */
    void Play(const std::string& animationName, bool loop = true, float fadeDuration = 0.0f);
    
    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override { return "AnimatorComponent"; }
    
    /**
     * @brief RawAnimator を取得する。
     * @return 取得された RawAnimator
     */
    Animator* GetRawAnimator() { return animator_.get(); }

private:
    std::unique_ptr<Animator> animator_;
    std::string defaultAnimation_ = "";
    std::string currentLoadedAnimation_ = "";
    float playbackSpeed_ = 1.0f;
    bool applyRootMotion_ = false;
};
