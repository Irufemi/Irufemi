#include "Core/GameLoopManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/SceneManager.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/TextRendererComponent.h"
#include "Framework/BaseScene.h"
#include "Engine/Irufemi.h"

#include "Player/GravityPlayerComponent.h"
#include "Combat/Boss/BossComponent.h"
#include "Engine/Core/Utility/Log.h"
#include <iostream>

void GameLoopManagerComponent::Initialize() {
    state_ = State::Playing;
    timer_ = 0.0f;
    player_ = nullptr;
    boss_ = nullptr;
    resultTextObj_ = nullptr;
    pressSpaceObj_ = nullptr;
}

void GameLoopManagerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Result Delay Time", &resultDelayTime_);
    RegisterProperty("Result Time Scale", &timeScaleAtResult_);
}

void GameLoopManagerComponent::Update() {
    auto engine = BaseModel::GetIrufemiEngine();

    if (state_ == State::Playing) {
        if (!player_) {
            auto playerObj = gameObject_->GetScene()->FindGameObject("Player");
            if (playerObj) player_ = playerObj->GetComponent<GravityPlayerComponent>();
        }
        if (!boss_) {
            auto bossObj = gameObject_->GetScene()->FindGameObject("Boss");
            if (bossObj) boss_ = bossObj->GetComponent<BossComponent>();
        }

        if (player_ && player_->IsDead()) {
            state_ = State::ResultTransition;
            isClear_ = false;
            engine->SetTimeScale(timeScaleAtResult_);
            ShowResultUI(false);
        }
        else if (boss_ && boss_->GetHp() <= 0) {
            state_ = State::ResultTransition;
            isClear_ = true;
            if (player_) {
                player_->SetGodMode(true); // ゲームクリア時に被弾しないようにする
            }
            engine->SetTimeScale(timeScaleAtResult_);
            ShowResultUI(true);
        }
    }
    else if (state_ == State::ResultTransition) {
        // スローモーションの影響を受けない実時間（RealDeltaTime）でカウントする
        float dt = engine->GetRealDeltaTime(); 
        timer_ += dt;

        if (timer_ >= resultDelayTime_) {
            state_ = State::WaitingForInput;
            
            auto uiObj = std::make_shared<GameObject>("ReturnText");
            gameObject_->GetScene()->AddGameObject(uiObj);
            auto t = uiObj->GetTransform();
            if (t) t->SetPosition({ 640.0f, 600.0f, 0.0f });
            auto text = uiObj->AddComponent<TextRendererComponent>();
            text->SetFontId("toro_glitch");
            text->SetText(L"Press SPACE to Return");
            text->SetBaseScale(20.0f);
            text->SetTopMost(true);
            text->SetAlignment(TextAlignment::Center); 
            
            uiObj->Initialize();
            pressSpaceObj_ = uiObj.get();
        }
    }
    else if (state_ == State::WaitingForInput) {
        if (engine->GetInputManager()->IsKeyPressed(VK_SPACE) || engine->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
            engine->SetTimeScale(1.0f);
            engine->GetSceneManager()->TransitionTo("Title", SceneTransition::Type::Fade, 1.0f);
        }
    }
}

void GameLoopManagerComponent::ShowResultUI(bool isClear) {
    std::string msg = "[GameLoopManager] ShowResultUI called! isClear=" + std::to_string(isClear) + "\n";
    Log::OutPutLog(std::cout, msg);

    auto uiObj = std::make_shared<GameObject>("ResultText");
    gameObject_->GetScene()->AddGameObject(uiObj);
    auto t = uiObj->GetTransform();
    if (t) t->SetPosition({ 640.0f, 340.0f, 0.0f });
    
    auto text = uiObj->AddComponent<TextRendererComponent>();
    text->SetFontId("toro_glitch");
    text->SetText(isClear ? L"STAGE CLEAR" : L"GAME OVER");
    text->SetBaseScale(40.0f);
    text->SetTopMost(true);
    
    text->SetColor(isClear ? Irufemi::Vector4{0.5f, 1.0f, 0.5f, 1.0f} : Irufemi::Vector4{1.0f, 0.2f, 0.2f, 1.0f});
    text->SetAlignment(TextAlignment::Center);

    uiObj->Initialize();
    
    resultTextObj_ = uiObj.get();
}
