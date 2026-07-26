#include "BossStateDestroyed.h"
#include "BossComponent.h"
#include "Engine/Core/Utility/Log.h"
#include <iostream>

void BossStateDestroyed::Enter(BossComponent* boss) {
    Log::OutPutLog(std::cout, "Boss Destroyed!\n");
}

void BossStateDestroyed::Update(BossComponent* boss) {
}

void BossStateDestroyed::Exit(BossComponent* boss) {
}

void BossStateDestroyed::OnTakeDamage(BossComponent* boss, float damage) {
}
