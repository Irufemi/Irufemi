#include "BossStateDestroyed.h"
#include "BossComponent.h"
#include "Framework/GameObject.h"
#include "Engine/Core/Utility/Log.h"
#include <iostream>

void BossStateDestroyed::Enter(BossComponent* boss) {
    Log::OutPutLog(std::cout, "Boss Destroyed!\n");
    if (boss && boss->gameObject_) {
        boss->gameObject_->SetIsActive(false);
    }
}

void BossStateDestroyed::Update(BossComponent* boss) {
}

void BossStateDestroyed::Exit(BossComponent* boss) {
}

void BossStateDestroyed::OnTakeDamage(BossComponent* boss, float damage) {
}
