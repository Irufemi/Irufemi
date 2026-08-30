#include "Core/GameLoopManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/TextRendererComponent.h"
#include "Renderer/Font/FontManager.h"
#include "Framework/Scene/BaseScene.h"
#include "Engine/Irufemi.h"
#include "Scenes/Result/ResultScene.h"

#include "Player/PlayerHealthComponent.h"
#include "Combat/Boss/BossComponent.h"
#include "Core/Utility/Log.h"
#include <iostream>

void GameLoopManagerComponent::Initialize() {
    state_ = State::Playing;
    playerHealth_ = nullptr;
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
    RegisterProperty("Result Time Scale", &timeScaleAtResult_);
    RegisterProperty("Target Player Name", &targetPlayerName_);
    RegisterProperty("Target Boss Name", &targetBossName_);
}

void GameLoopManagerComponent::Update() {
    if (state_ == State::Playing) {
        if (!playerHealth_ && !targetPlayerName_.empty()) {
            auto playerObj = gameObject_->GetScene()->FindGameObject(targetPlayerName_);
            if (playerObj) {
                playerHealth_ = playerObj->GetComponent<PlayerHealthComponent>();
                if (playerHealth_) {
                    playerHealth_->onPlayerDied = [this]() { OnPlayerDied(); };
                    playerHealth_->onDeathSequenceFinished = [this]() { OnDeathSequenceFinished(); };
                }
            }
        }
        if (!boss_ && !targetBossName_.empty()) {
            auto bossObj = gameObject_->GetScene()->FindGameObject(targetBossName_);
            if (bossObj) {
                boss_ = bossObj->GetComponent<BossComponent>();
                if (boss_) {
                    boss_->onBossDied = [this]() { OnBossDied(); };
                    boss_->onDeathSequenceFinished = [this]() { OnDeathSequenceFinished(); };
                }
            }
        }
    }
}

void GameLoopManagerComponent::OnBossDied() {
    if (state_ != State::Playing)
        return;
    state_ = State::Finished;
    isClear_ = true;
    if (playerHealth_) {
        playerHealth_->SetGodMode(true); // ゲームクリア時に被弾しないようにする
    }
    BaseModel::GetIrufemiEngine()->SetTimeScale(timeScaleAtResult_);
}

void GameLoopManagerComponent::OnPlayerDied() {
    if (state_ != State::Playing)
        return;
    state_ = State::Finished;
    isClear_ = false;
    BaseModel::GetIrufemiEngine()->SetTimeScale(timeScaleAtResult_);
}

void GameLoopManagerComponent::OnDeathSequenceFinished() {
    ResultScene::s_isClear = isClear_;
    BaseModel::GetIrufemiEngine()->GetSceneManager()->PushScene("Result");
}
