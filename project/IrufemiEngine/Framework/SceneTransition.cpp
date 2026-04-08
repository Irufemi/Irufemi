#include "SceneTransition.h"
#include <algorithm>

void SceneTransition::Initialize(PostProcessManager* ppManager) {
    ppManager_ = ppManager;
}

void SceneTransition::Start(Type type, float duration, bool isOut) {
    if (!ppManager_) return;

    currentType_ = type;
    duration_ = (std::max)(0.001f, duration); // 0除算防止
    isOut_ = isOut;
    timer_ = 0.0f;
    isActive_ = true;

    // 演出開始時に既存のポストプロセスをリセットし、必要なモードを追加
    ppManager_->ClearActiveModes();
    switch (currentType_) {
    case Type::Fade:
        ppManager_->AddActiveMode(PostProcessMode::Fade);
        break;
    case Type::Dissolve:
        ppManager_->AddActiveMode(PostProcessMode::Dissolve);
        break;
    case Type::Slide:
        ppManager_->AddActiveMode(PostProcessMode::Slide);
        break;
    case Type::RadialBlur:
        // 放射状ブラーとフェードを併用して、暗くなりつつボケる演出にする
        ppManager_->AddActiveMode(PostProcessMode::RadialBlur);
        ppManager_->AddActiveMode(PostProcessMode::Fade);
        break;
    }
}

void SceneTransition::Update(float deltaTime) {
    if (!isActive_ || !ppManager_) return;

    timer_ += deltaTime;
    float totalDuration = duration_ + kDwellTime;

    if (timer_ >= totalDuration) {
        timer_ = totalDuration;
        isActive_ = false;
        
        // フェードイン（画面が表示される方）が完了した場合はポストプロセスをクリア
        if (!isOut_) {
            ppManager_->ClearActiveModes();
        }
    }

    // 演出自体の進行度 (0.0 ~ 1.0) 
    // 溜め時間 (kDwellTime) 中は 1.0 固定にする
    float progress = (std::min)(1.0f, timer_ / duration_);
    
    // 実際にエフェクトに適用する係数
    float factor = isOut_ ? progress : (1.0f - progress);

    // 各モードのパラメータに反映
    switch (currentType_) {
    case Type::Fade:
        ppManager_->GetFadeParams().intensity = factor;
        break;
    case Type::Dissolve:
        // 1.1 まで動かすことでノイズを確実に消し去る
        ppManager_->GetDissolveParams().threshold = factor * 1.1f;
        break;
    case Type::Slide:
        // 1.05 まで動かすことで境界のボケ(edgeWidth=0.02)を画面外へ完全に追いやる
        ppManager_->GetSlideParams().threshold = factor * 1.05f;
        break;
    case Type::RadialBlur:
        ppManager_->GetRadialBlurParams().blurWidth = factor * 0.05f;
        ppManager_->GetFadeParams().intensity = factor;
        break;
    }
}
