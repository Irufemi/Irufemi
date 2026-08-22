#include "Combat/Boss/BossStateDestroyed.h"
#include "Combat/Boss/BossComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Core/Utility/Log.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/Component/Camera/CameraShakeComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/SkinnedMeshRendererComponent.h"
#include <iostream>

void BossStateDestroyed::Enter(BossComponent* boss) {
    Log::OutPutLog(std::cout, "Boss Destroyed!\n");
    hasFinished_ = false;
    shakeComp_ = nullptr;

    if (boss && boss->gameObject_) {
        // ボス破壊時の特大シェイク
        if (auto scene = boss->gameObject_->GetScene()) {
            if (auto mainCameraObj = scene->FindGameObject("MainCamera")) {
                shakeComp_ = mainCameraObj->GetComponent<CameraShakeComponent>();
                if (shakeComp_) {
                    shakeComp_->PlayShake(2.0f, 60, 10.0f); // Intensity=2.0, 60 Frames, Freq=10 (大きめ、ゆっくり)
                }
            }
        }
        
        if (boss->onBossDied) {
            boss->onBossDied();
        }

        // 演出エフェクトが始まったタイミングでボスのモデル描画をすべて切る
        auto renderers = boss->gameObject_->GetComponentsInChildren<MeshRendererComponent>();
        for (auto* r : renderers) {
            if (r) r->SetVisible(false);
        }
        auto skinnedRenderers = boss->gameObject_->GetComponentsInChildren<SkinnedMeshRendererComponent>();
        for (auto* r : skinnedRenderers) {
            if (r) r->SetVisible(false);
        }
    }
}

void BossStateDestroyed::Update(BossComponent* boss) {
    if (hasFinished_) return;

    if (shakeComp_) {
        if (!shakeComp_->IsPlaying()) {
            hasFinished_ = true;
            if (boss && boss->gameObject_) {
                boss->gameObject_->SetIsActive(false);
            }
            if (boss && boss->onDeathSequenceFinished) {
                boss->onDeathSequenceFinished();
            }
        }
    } else {
        hasFinished_ = true;
        if (boss && boss->gameObject_) {
            boss->gameObject_->SetIsActive(false);
        }
        if (boss && boss->onDeathSequenceFinished) {
            boss->onDeathSequenceFinished();
        }
    }
}

void BossStateDestroyed::Exit(BossComponent* boss) {
}

void BossStateDestroyed::OnTakeDamage(BossComponent* boss, float damage) {
}
