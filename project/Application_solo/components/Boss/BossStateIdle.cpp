#include "BossStateIdle.h"
#include "BossStateCoreExposed.h"
#include "BossComponent.h"
#include "../EnemyBeamComponent.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Core/Utility/Log.h"
#include <iostream>
#include <memory>

void BossStateIdle::Enter(BossComponent* boss) {
    Log::OutPutLog(std::cout, "Boss entered Idle State (Shield Active)\n");
}

void BossStateIdle::Update(BossComponent* boss) {
    if (!boss->gameObject_) return;

    // --- ビーム攻撃のタイマー処理 ---
    if (boss->beamComponent_) {
        float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
        if (deltaTime <= 0.0f) deltaTime = 1.0f / 60.0f;
        
        if (!boss->beamComponent_->IsActive()) {
            boss->beamTimer_ += deltaTime;
            if (boss->beamTimer_ >= boss->beamInterval_) {
                boss->beamTimer_ = 0.0f;
                
                if (auto myTrans = boss->gameObject_->GetComponent<TransformComponent>()) {
                    Irufemi::Vector3 startPos = myTrans->GetWorldPosition();
                    
                    Irufemi::Matrix4x4 worldMat = myTrans->GetWorldMatrix();
                    Irufemi::Vector3 forward = { -worldMat.m[2][0], -worldMat.m[2][1], -worldMat.m[2][2] };
                    forward = Irufemi::Math::Normalize(forward);
                    
                    startPos = Irufemi::Math::Add(startPos, Irufemi::Math::Multiply(boss->beamOffsetZ_, forward)); 
                    startPos.y += boss->beamOffsetY_;
                    
                    Irufemi::Vector3 targetPos = Irufemi::Math::Add(startPos, Irufemi::Math::Multiply(boss->beamRange_, forward));
                    boss->beamComponent_->Fire(startPos, targetPos);
                }
            }
        }
    }
    
    // CoreExposed への遷移チェック
    if (boss->isShieldsInitialized_ && boss->initialShieldsSpawned_ > 0 && boss->shields_.empty()) {
        boss->ChangeState(std::make_unique<BossStateCoreExposed>());
    }
}

void BossStateIdle::Exit(BossComponent* boss) {
}

void BossStateIdle::OnTakeDamage(BossComponent* boss, float damage) {
    Log::OutPutLog(std::cout, "Boss blocked damage with shield!\n");
}
