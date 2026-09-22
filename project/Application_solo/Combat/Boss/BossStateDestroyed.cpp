#include "Combat/Boss/BossStateDestroyed.h"
#include "Combat/Boss/BossComponent.h"
#include "Combat/Boss/BossDamageVisualizerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Core/Utility/Log.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/SkinnedMeshRendererComponent.h"
#include <iostream>

void BossStateDestroyed::Enter(BossComponent* boss) {
    Log::OutPutLog(std::cout, "Boss Destroyed!\n");
    hasFinished_ = false;
    visualizer_ = nullptr;

    if (boss && boss->gameObject_) {
        visualizer_ = boss->gameObject_->GetComponent<BossDamageVisualizerComponent>();

        // 撃破イベントの通知（BossDamageVisualizerComponent が特大カメラシェイクを発火）
        boss->NotifyBossDied();

        // 演出エフェクトが始まったタイミングでボスのモデル描画をすべて切る
        auto renderers = boss->gameObject_->GetComponentsInChildren<MeshRendererComponent>();
        for (auto* r : renderers) {
            if (r) {
                r->SetVisible(false);
            }
        }
        auto skinnedRenderers = boss->gameObject_->GetComponentsInChildren<SkinnedMeshRendererComponent>();
        for (auto* r : skinnedRenderers) {
            if (r) {
                r->SetVisible(false);
            }
        }
    }
}

void BossStateDestroyed::Update(BossComponent* boss) {
    if (hasFinished_) {
        return;
    }

    bool isShakePlaying = visualizer_ ? visualizer_->IsDeathShakePlaying() : false;

    if (!isShakePlaying) {
        hasFinished_ = true;
        if (boss && boss->gameObject_) {
            boss->gameObject_->SetIsActive(false);
        }
        if (boss) {
            boss->NotifyDeathSequenceFinished();
        }
    }
}

void BossStateDestroyed::Exit(BossComponent* boss) {}

void BossStateDestroyed::OnTakeDamage(BossComponent* boss, float damage) {}
