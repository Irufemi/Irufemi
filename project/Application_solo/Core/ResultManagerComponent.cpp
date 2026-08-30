#include "Core/ResultManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/TextRendererComponent.h"
#include "Scenes/Result/ResultScene.h"
#include "Engine/Irufemi.h"
#include <algorithm>
#include <cmath>

void ResultManagerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Result Delay Time", &resultDelayTime_);
    RegisterProperty("Time Scale Reset Delay", &timeScaleResetDelayTime_);
    RegisterProperty("Time Scale Recovery Duration", &timeScaleRecoveryDuration_);
    RegisterProperty("Next Scene Name", &nextSceneName_);
}

void ResultManagerComponent::Initialize() {
    timer_ = 0.0f;
    canReturnToTitle_ = false;
    hasResetTimeScale_ = false;
    isRecoveringTimeScale_ = false;
    timeScaleRecoveryTimer_ = 0.0f;
    startTimeScale_ = 0.0f;

    // 自分自身のGameObjectに TextRendererComponent を追加（または既存のものを取得）
    auto text = gameObject_->GetComponent<TextRendererComponent>();
    if (!text) {
        text = gameObject_->AddComponent<TextRendererComponent>().get();
    }
    
    // ResultScene に渡された静的フラグを使って表示文字を決定
    text->SetFontId("toro_glitch");
    text->SetText(ResultScene::s_isClear ? L"STAGE CLEAR" : L"GAME OVER");
    text->SetTopMost(true);
    text->SetAlignment(TextAlignment::Center);
    // クリア時は緑っぽく、失敗時は赤っぽくするなど
    text->SetColor(ResultScene::s_isClear ? Irufemi::Vector4{0.5f, 1.0f, 0.5f, 1.0f} : Irufemi::Vector4{1.0f, 0.2f, 0.2f, 1.0f});
}

void ResultManagerComponent::Update() {
    auto engine = BaseModel::GetIrufemiEngine();

    // スローモーションの影響を受けないよう、RealDeltaTimeを使用する
    float dt = engine->GetRealDeltaTime();
    timer_ += dt;

    // 一定時間経過後、スローモーションを解除して等倍速に戻す（イージング開始）
    if (!hasResetTimeScale_ && !isRecoveringTimeScale_ && timer_ >= timeScaleResetDelayTime_) {
        isRecoveringTimeScale_ = true;
        startTimeScale_ = engine->GetTimeScale();
        timeScaleRecoveryTimer_ = 0.0f;
    }

    // イージング中の処理
    if (isRecoveringTimeScale_) {
        timeScaleRecoveryTimer_ += dt;
        float duration = (std::max)(timeScaleRecoveryDuration_, 0.001f);
        float t = std::clamp(timeScaleRecoveryTimer_ / duration, 0.0f, 1.0f);
        
        // 滑らかに補間する (Smoothstep: 3t^2 - 2t^3)
        float easeT = t * t * (3.0f - 2.0f * t);
        
        // C++20 std::lerp (無い場合は std::lerp互換の計算: a + (b-a)*t)
        float newScale = startTimeScale_ + (1.0f - startTimeScale_) * easeT;
        engine->SetTimeScale(newScale);

        if (t >= 1.0f) {
            isRecoveringTimeScale_ = false;
            hasResetTimeScale_ = true;
            engine->SetTimeScale(1.0f); // 確実に1.0に戻す
        }
    }

    if (!canReturnToTitle_ && timer_ >= resultDelayTime_) {
        canReturnToTitle_ = true;

        // "Press SPACE to Return" 用のテキストオブジェクトを動的生成
        auto uiObj = std::make_shared<GameObject>("ReturnText");
        gameObject_->GetScene()->AddGameObject(uiObj);
        auto t = uiObj->GetTransform();
        if (t) t->SetPosition({ 640.0f, 600.0f, 0.0f });

        auto text = uiObj->AddComponent<TextRendererComponent>().get();
        text->SetFontId("toro_glitch");
        text->SetText(L"Press SPACE to Return");
        text->SetBaseScale(20.0f);
        text->SetTopMost(true);
        text->SetAlignment(TextAlignment::Center);

        uiObj->Initialize();
        pressSpaceObj_ = uiObj.get();
    }

    if (canReturnToTitle_) {
        if (engine->GetInputManager()->IsKeyPressed(VK_SPACE) || 
            engine->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
            
            engine->SetTimeScale(1.0f);
            engine->GetSceneManager()->TransitionTo(nextSceneName_, SceneTransition::Type::Fade, 1.0f);
        }
    }
}
