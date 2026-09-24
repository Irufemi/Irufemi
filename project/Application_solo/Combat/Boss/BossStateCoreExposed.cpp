#include "Combat/Boss/BossStateCoreExposed.h"
#include "Combat/Boss/BossStateDestroyed.h"
#include "Combat/Boss/BossComponent.h"
#include "Core/Utility/Log.h"
#include <iostream>
#include <memory>
#include <string>
#include "Framework/GameObject/GameObject.h"

void BossStateCoreExposed::Enter(BossComponent* boss) {
    Log::OutPutLog(std::cout, "Boss entered CoreExposed State (Vulnerable)\n");
}

void BossStateCoreExposed::Update(BossComponent* boss) {}

void BossStateCoreExposed::Exit(BossComponent* boss) {}

void BossStateCoreExposed::OnTakeDamage(BossComponent* boss, float damage) {
    boss->hp_ -= damage;

    std::string dmgLog = "Boss took damage! HP: " + std::to_string(boss->hp_) + "\n";
    Log::OutPutLog(std::cout, dmgLog);

    // 被弾イベント通知（演出コンポーネントがカメラシェイク等を担当）
    boss->NotifyDamageTaken(damage);

    if (boss->hp_ <= 0) {
        boss->hp_ = 0;
        boss->ChangeState(std::make_unique<BossStateDestroyed>());
    }
}
