#include "BossStateCoreExposed.h"
#include "BossStateDestroyed.h"
#include "BossComponent.h"
#include "Engine/Core/Utility/Log.h"
#include <iostream>
#include <memory>
#include <string>

void BossStateCoreExposed::Enter(BossComponent* boss) {
    Log::OutPutLog(std::cout, "Boss entered CoreExposed State (Vulnerable)\n");
}

void BossStateCoreExposed::Update(BossComponent* boss) {
}

void BossStateCoreExposed::Exit(BossComponent* boss) {
}

void BossStateCoreExposed::OnTakeDamage(BossComponent* boss, float damage) {
    boss->hp_ -= damage;
    
    std::string dmgLog = "Boss took damage! HP: " + std::to_string(boss->hp_) + "\n";
    Log::OutPutLog(std::cout, dmgLog);

    if (boss->hp_ <= 0) {
        boss->hp_ = 0;
        boss->ChangeState(std::make_unique<BossStateDestroyed>());
    }
}
