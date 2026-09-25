#include "Combat/Boss/BossStateCoreExposed.h"
#include "Combat/Boss/BossStateDestroyed.h"
#include "Combat/Boss/BossComponent.h"
#include "Environment/DebrisManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Core/Utility/Log.h"
#include <iostream>
#include <memory>
#include <string>

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

    // ボス装甲の被弾剥離（破片ドロップ連携）：弾丸ヒット時に破片を2個飛散させる
    if (boss->debrisManager_ && boss->GetGameObject()) {
        Irufemi::Vector3 bossPos = boss->GetGameObject()->GetTransform()->GetWorldPosition();
        boss->debrisManager_->SpawnDebrisCluster(bossPos, 2, 4.0f);
    }

    if (boss->hp_ <= 0) {
        boss->hp_ = 0;
        boss->ChangeState(std::make_unique<BossStateDestroyed>());
    }
}
