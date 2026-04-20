#include "Phase2.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "Engine/Core/Math/Math.h"

void Phase2::Enter(Enemy* enemy) {
    for (int i = 0; i < 3; ++i) {
        currentModes_[i] = Mode::Idle;

        idleStates_[i] = std::make_unique<Phase2_Idle>();
        idleStates_[i]->SetHeadIndex(i);
        idleStates_[i]->Enter(enemy);

        beamStates_[i] = std::make_unique<Phase2_Beam>();
        beamStates_[i]->SetHeadIndex(i);

        biteStates_[i] = std::make_unique<Phase2_Bite>();
        biteStates_[i]->SetHeadIndex(i);
    }
}

void Phase2::Update(Enemy* enemy, Player* player, float deltaTime) {
    for (int i = 0; i < 3; ++i) {
        if (enemy->IsHeadDead(i)) continue; // 死んだ頭の行動は停止
        
        if (currentModes_[i] == Mode::Idle) {
            idleStates_[i]->Update(enemy, player, deltaTime);

            if (idleStates_[i]->WantsToBite()) {
                idleStates_[i]->Exit(enemy);
                currentModes_[i] = Mode::Biting;
                biteStates_[i]->Enter(enemy);
            } else if (idleStates_[i]->WantsToBeam()) {
                idleStates_[i]->Exit(enemy);
                currentModes_[i] = Mode::Beaming;
                beamStates_[i]->Enter(enemy);
            }
        } else if (currentModes_[i] == Mode::Biting) {
            biteStates_[i]->Update(enemy, player, deltaTime);
            if (biteStates_[i]->IsFinished()) {
                biteStates_[i]->Exit(enemy);
                currentModes_[i] = Mode::Idle;
                idleStates_[i]->Enter(enemy);
            }
        } else if (currentModes_[i] == Mode::Beaming) {
            beamStates_[i]->Update(enemy, player, deltaTime);
            if (beamStates_[i]->IsFinished()) {
                beamStates_[i]->Exit(enemy);
                currentModes_[i] = Mode::Idle;
                idleStates_[i]->Enter(enemy);
            }
        }
    }

    ApplyRepulsion(enemy);
}

void Phase2::Exit(Enemy* enemy) {
    for (int i = 0; i < 3; ++i) {
        if (currentModes_[i] == Mode::Idle) idleStates_[i]->Exit(enemy);
        else if (currentModes_[i] == Mode::Biting) biteStates_[i]->Exit(enemy);
        else if (currentModes_[i] == Mode::Beaming) beamStates_[i]->Exit(enemy);
    }
}

void Phase2::ApplyRepulsion(Enemy* enemy) {
    Transform* transforms[3] = {
        &enemy->GetHeadLeftLocalTransform(),
        &enemy->GetHeadMidLocalTransform(),
        &enemy->GetHeadRightLocalTransform()
    };

    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 3; ++j) {
            if (enemy->IsHeadDead(i) || enemy->IsHeadDead(j)) continue; // 死んだ頭とは反発しない

            Vector3 diff = Math::Subtract(transforms[i]->translate, transforms[j]->translate);
            float dist = Math::Length(diff);
            if (dist < kRepulsionRadius && dist > 0.001f) {
                float strength = (kRepulsionRadius - dist) / kRepulsionRadius;
                Vector3 push = Math::Multiply(kRepulsionForceScale * strength * strength, Math::Normalize(diff));
                
                // 待機状態の首にのみ反発速度を加算
                if (currentModes_[i] == Mode::Idle) {
                    Vector3 v = idleStates_[i]->GetVelocity();
                    idleStates_[i]->SetVelocity(Math::Add(v, push));
                }
                if (currentModes_[j] == Mode::Idle) {
                    Vector3 v = idleStates_[j]->GetVelocity();
                    idleStates_[j]->SetVelocity(Math::Subtract(v, push));
                }
            }
        }
    }
}
