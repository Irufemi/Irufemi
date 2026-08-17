#include "Combat/Boss/BossStateDestroyed.h"
#include "Combat/Boss/BossComponent.h"
#include "Framework/GameObject.h"
#include "Engine/Core/Utility/Log.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/Camera/CameraShakeComponent.h"
#include <iostream>

void BossStateDestroyed::Enter(BossComponent* boss) {
    Log::OutPutLog(std::cout, "Boss Destroyed!\n");
    if (boss && boss->gameObject_) {
        // ボス破壊時の特大シェイク
        if (auto scene = boss->gameObject_->GetScene()) {
            if (auto mainCameraObj = scene->FindGameObject("MainCamera")) {
                if (auto shakeComp = mainCameraObj->GetComponent<CameraShakeComponent>()) {
                    shakeComp->PlayShake(2.0f, 60, 10.0f); // Intensity=2.0, 60 Frames, Freq=10 (大きめ、ゆっくり)
                }
            }
        }
        boss->gameObject_->SetIsActive(false);
    }
}

void BossStateDestroyed::Update(BossComponent* boss) {
}

void BossStateDestroyed::Exit(BossComponent* boss) {
}

void BossStateDestroyed::OnTakeDamage(BossComponent* boss, float damage) {
}
