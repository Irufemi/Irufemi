#include "Core/GameLoopManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/TextRendererComponent.h"
#include "Renderer/Font/FontManager.h"
#include "Framework/Scene/BaseScene.h"
#include "Engine/Irufemi.h"
#include "Scenes/Result/ResultScene.h"

#include "Player/GravityPlayerComponent.h"
#include "Combat/Boss/BossComponent.h"
#include "Core/Utility/Log.h"
#include <iostream>

void GameLoopManagerComponent::Initialize() {
    state_ = State::Playing;
    timer_ = 0.0f;
    player_ = nullptr;
    boss_ = nullptr;

    // 事前キャッシュ: ResultScene で使用するテキストのSDF生成をバックグラウンドで事前に行う
    if (auto engine = BaseModel::GetIrufemiEngine()) {
        if (auto fm = engine->GetFontManager()) {
            fm->PrecacheText("toro_glitch", L"STAGE CLEAR");
            fm->PrecacheText("toro_glitch", L"GAME OVER");
            fm->PrecacheText("toro_glitch", L"Press SPACE to Return");
        }
    }
}

void GameLoopManagerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Result Delay Time", &resultDelayTime_);
    RegisterProperty("Result Time Scale", &timeScaleAtResult_);
    RegisterProperty("Target Player Name", &targetPlayerName_);
    RegisterProperty("Target Boss Name", &targetBossName_);
}

void GameLoopManagerComponent::Update() {
    auto engine = BaseModel::GetIrufemiEngine();

    if (state_ == State::Playing) {
        if (!player_ && !targetPlayerName_.empty()) {
            auto playerObj = gameObject_->GetScene()->FindGameObject(targetPlayerName_);
            if (playerObj) player_ = playerObj->GetComponent<GravityPlayerComponent>();
        }
        if (!boss_ && !targetBossName_.empty()) {
            auto bossObj = gameObject_->GetScene()->FindGameObject(targetBossName_);
            if (bossObj) boss_ = bossObj->GetComponent<BossComponent>();
        }

        if (player_ && player_->IsDead()) {
            state_ = State::Finished;
            isClear_ = false;
            ResultScene::s_isClear = false;
            engine->SetTimeScale(timeScaleAtResult_);
            engine->GetSceneManager()->PushScene("Result");
        }
        else if (boss_ && boss_->GetHp() <= 0) {
            state_ = State::Finished;
            isClear_ = true;
            if (player_) {
                player_->SetGodMode(true); // ゲームクリア時に被弾しないようにする
            }
            ResultScene::s_isClear = true;
            engine->SetTimeScale(timeScaleAtResult_);
            engine->GetSceneManager()->PushScene("Result");
        }
    }
}
