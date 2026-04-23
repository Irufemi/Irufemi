#include "Phase1_NeckAttack.h"
#include "actors/enemy/Enemy.h"
#include "actors/player/Player.h"
#include "Engine/Core/Math/Math.h"
#include <cmath>

void Phase1_NeckAttack::Enter(Enemy* enemy) {
    totalTimer_ = 0.0f;
    currentAttackIndex_ = 0;
    hasFinishedAttack_ = false;
    currentPhase_ = AttackPhase::WindUp;
    phaseTimer_ = 0.0f;

    // 初期化：各部位のオフセットと回転をリセット
    for (int i = 0; i < 3; ++i) {
        enemy->GetBodyOffset(i) = {0,0,0};
    }
    enemy->GetHeadLeftOffset() = {0,0,0};
    enemy->GetHeadRightOffset() = {0,0,0};
    enemy->GetHeadMidOffset() = {0,0,0};

    enemy->GetHeadLeftLocalTransform().rotate = {0,0,0};
    enemy->GetHeadRightLocalTransform().rotate = {0,0,0};
    enemy->GetHeadMidLocalTransform().rotate = {0,0,0};
    
    enemy->GetHeadLeftLocalTransform().scale = {1,1,1};
    enemy->GetHeadRightLocalTransform().scale = {1,1,1};
    enemy->GetHeadMidLocalTransform().scale = {1,1,1};
}

void Phase1_NeckAttack::Update(Enemy* enemy, Player* player, float deltaTime) {
    totalTimer_ += deltaTime;
    phaseTimer_ += deltaTime;

    auto lerp = [](float start, float end, float t) {
        return start + (end - start) * t;
    };
    auto lerpVec3 = [&](const Vector3& start, const Vector3& end, float t) {
        return Vector3{lerp(start.x, end.x, t), lerp(start.y, end.y, t), lerp(start.z, end.z, t)};
    };

    // --- スイング方向の決定 ---
    float swingDir = (currentAttackIndex_ == 1) ? -1.0f : 1.0f;

    // --- フェーズ進行管理 ---
    float currentPhaseDuration = 0.0f;
    if (currentPhase_ == AttackPhase::WindUp) currentPhaseDuration = windUpTime_;
    else if (currentPhase_ == AttackPhase::Sweep) currentPhaseDuration = sweepTime_;
    else if (currentPhase_ == AttackPhase::Recovery) currentPhaseDuration = recoveryTime_;
    else if (currentPhase_ == AttackPhase::ReturnToIdle) currentPhaseDuration = returnToIdleTime_;

    float progress = (currentPhaseDuration > 0.0f) ? std::clamp(phaseTimer_ / currentPhaseDuration, 0.0f, 1.0f) : 0.0f;
    float easeOut = 1.0f - std::pow(1.0f - progress, 3.0f); // イーズアウト
    float easeIn = std::pow(progress, 3.0f); // イーズイン

    // --- プレイヤーの追従および突進 ---
    if (player) {
        Vector3 playerPos = player->GetTranslate();
        Vector3 enemyPos = enemy->GetGlobalTransform().translate;
        Vector3 diff = Math::Subtract(playerPos, enemyPos);
        diff.y = 0.0f; // 高さを無視

        // 進行方向：WindUp中のみプレイヤーを狙う。Sweep中は向いている方向を固定
        Vector3 dir = {0,0,0};
        
        if (currentPhase_ == AttackPhase::WindUp) {
            float dist = Math::Length(diff);
            if (dist > 0.001f) {
                dir = Math::Normalize(diff);
                
                // 首振り：Y軸でプレイヤーの方向を少しずつ向く
                float targetYaw = std::atan2(dir.x, dir.z);
                float currentYaw = enemy->GetGlobalTransform().rotate.y;
                
                float diffYaw = targetYaw - currentYaw;
                while (diffYaw > 3.14159f) diffYaw -= 2.0f * 3.14159f;
                while (diffYaw < -3.14159f) diffYaw += 2.0f * 3.14159f;

                enemy->GetGlobalTransform().rotate.y += diffYaw * trackRotSpeed_ * deltaTime;
            }
        }

        // 移動処理：WindUp中もSweep中も前進するが、Sweep中は「現在の体の向き」へ直進する
        float moveSpeed = 0.0f;
        if (currentPhase_ == AttackPhase::WindUp) {
            moveSpeed = trackSpeedWindUp_;
            // dirは上で計算されたプレイヤー方向（もし向いていれば）
            if (Math::Length(dir) < 0.001f) {
                dir.x = std::sin(enemy->GetGlobalTransform().rotate.y);
                dir.z = std::cos(enemy->GetGlobalTransform().rotate.y);
            }
        } else if (currentPhase_ == AttackPhase::Sweep) {
            moveSpeed = trackSpeedSweep_;
            // 攻撃開始後はプレイヤーを追走せず、今の体の向きへ一直線に突進させる（回避可能にするため）
            dir.x = std::sin(enemy->GetGlobalTransform().rotate.y);
            dir.z = std::cos(enemy->GetGlobalTransform().rotate.y);
        }
        
        if (moveSpeed > 0.0f) {
            enemy->GetGlobalTransform().translate = Math::Add(enemy->GetGlobalTransform().translate, Math::Multiply(moveSpeed * deltaTime, dir));
        }
    }

    // --- ボディのアニメーション（極端な潰れ＋常にかくねくね這い寄る蛇） ---
    float bodyTargetRatio = 0.0f; 
    if (currentPhase_ == AttackPhase::WindUp) {
        bodyTargetRatio = easeOut;
    } else if (currentPhase_ == AttackPhase::Sweep) {
        bodyTargetRatio = 1.0f;
    } else {
        bodyTargetRatio = 1.0f - easeOut; 
    }

    for (int i = 0; i < 3; ++i) {
        float fI = (float)(i + 1) / 3.0f;
        Vector3 targetBodyOffset = {0,0,0};
        
        // 【重要】常時くねくねさせるウェーブ運動
        float wiggle = std::sin(totalTimer_ * 12.0f - i * 1.5f) * 1.8f * fI;
        targetBodyOffset.x = wiggle;

        if (currentPhase_ == AttackPhase::ReturnToIdle) {
            targetBodyOffset = {0.0f, 0.0f, 0.0f};
        } else {
            // ヘッダーのパラメータで体全体を上下させる
            targetBodyOffset.y = bodySquashDepth_ * fI + bodyHeightOffset_; 

            if (currentPhase_ == AttackPhase::WindUp) {
                targetBodyOffset.z = -0.5f * fI * easeOut; 
            } else if (currentPhase_ == AttackPhase::Sweep) {
                targetBodyOffset.z = 3.5f * fI * easeIn; // 前方へ鋭く突進！
            } else {
                targetBodyOffset.z = 3.5f * fI * (1.0f - easeOut);
            }
        }
        
        Vector3& currentBodyOffset = enemy->GetBodyOffset(i);
        currentBodyOffset = lerpVec3(currentBodyOffset, targetBodyOffset, 0.3f);
    }

    // --- 頭のアニメーション ---
    // まずすべての頭のターゲットを初期値で用意
    Vector3 targetOffsets[3] = {{0,0,0}, {0,0,0}, {0,0,0}};
    Vector3 targetRotates[3] = {{0,0,0}, {0,0,0}, {0,0,0}};
    Vector3 targetScales[3] = {{1,1,1}, {1,1,1}, {1,1,1}};

    // アクティブな頭だけアニメーションを適用
    float currentNeckLength = lerp(1.0f, neckStretchScale_, bodyTargetRatio);
    int active = currentAttackIndex_;

    // Recoveryフェーズのための戻り補間率
    float recoveryProgress = (currentPhase_ == AttackPhase::Recovery) ? std::clamp(phaseTimer_ / currentPhaseDuration, 0.0f, 1.0f) : 0.0f;

    if (currentPhase_ == AttackPhase::ReturnToIdle) {
        // すべての頭を完全にリセット（デフォルト位置へ）
        for (int i = 0; i < 3; ++i) {
            targetScales[i] = {1.0f, 1.0f, 1.0f};
            targetRotates[i] = {0.0f, 0.0f, 0.0f};
            targetOffsets[i] = {0.0f, 0.0f, 0.0f};
        }
    } else if (currentPhase_ == AttackPhase::WindUp || currentPhase_ == AttackPhase::Sweep || currentPhase_ == AttackPhase::Recovery) {
        
        // 攻撃中のスケールと回転のターゲット値の算出
        Vector3 currentTargetScale = {1.0f, 1.0f, 1.0f};
        Vector3 currentTargetRotate = {0.0f, 0.0f, 0.0f};

        if (active == 0) {   // LeftHead
            if (currentPhase_ == AttackPhase::WindUp || currentPhase_ == AttackPhase::Recovery) {
                currentTargetScale = {neckThickness_, currentNeckLength * hitRangeScale_, neckThickness_};
                currentTargetRotate.z = -1.0f * bodyTargetRatio; // 右斜め上に振りかぶる（元2連目）
                currentTargetRotate.x = -0.8f * bodyTargetRatio; 
            } else if (currentPhase_ == AttackPhase::Sweep) {
                currentTargetScale = {neckThickness_, neckStretchScale_ * hitRangeScale_, neckThickness_};
                currentTargetRotate.z = lerp(-1.0f, 0.5f, easeIn); // 右上から左斜め下へ
                currentTargetRotate.x = lerp(-0.8f, 2.4f, easeIn); // もうちょい下まで深く振り抜く！
            }
        } else if (active == 1) { // RightHead
            if (currentPhase_ == AttackPhase::WindUp || currentPhase_ == AttackPhase::Recovery) {
                currentTargetScale = {neckThickness_, currentNeckLength * hitRangeScale_, neckThickness_};
                currentTargetRotate.z = 1.0f * bodyTargetRatio;  // 左斜め上に振りかぶる（元1連目）
                currentTargetRotate.x = -0.8f * bodyTargetRatio; 
            } else if (currentPhase_ == AttackPhase::Sweep) {
                currentTargetScale = {neckThickness_, neckStretchScale_ * hitRangeScale_, neckThickness_};
                currentTargetRotate.z = lerp(1.0f, -0.5f, easeIn); // 左上から右斜め下へ
                currentTargetRotate.x = lerp(-0.8f, 2.4f, easeIn); // もうちょい下まで深く振り抜く！
            }
        } else if (active == 2) { // Mid
            if (currentPhase_ == AttackPhase::WindUp || currentPhase_ == AttackPhase::Recovery) {
                currentTargetScale = {neckThickness_, currentNeckLength * hitRangeScale_, neckThickness_};
                currentTargetRotate.x = 0.5f * bodyTargetRatio; // 振りかぶらずに、やや前傾のまま待機
            } else if (currentPhase_ == AttackPhase::Sweep) {
                currentTargetScale = {neckThickness_, neckStretchScale_ * hitRangeScale_, neckThickness_};
                currentTargetRotate.x = lerp(0.5f, 2.0f, easeIn); // 地面を突き抜けないよう、適度な角度（地面スレスレ）でピタッと止める
            }
        }

        // Recoveryフェーズ中は上記の「限界まで伸びた状態」から「{1,1,1}」や「{0,0,0}」へ時間に応じて明示的に戻していく
        if (currentPhase_ == AttackPhase::Recovery) {
            // スケールが2倍等になっていても、(1-t)倍して強制的に1倍(通常サイズ)に収束させる
            targetScales[active] = lerpVec3(currentTargetScale, {1.0f, 1.0f, 1.0f}, recoveryProgress);
            targetRotates[active] = lerpVec3(currentTargetRotate, {0.0f, 0.0f, 0.0f}, recoveryProgress);
            targetOffsets[active].y = lerp(attackHeadOffsetY_, 0.0f, recoveryProgress);
        } else {
            targetScales[active] = currentTargetScale;
            targetRotates[active] = currentTargetRotate;
            targetOffsets[active].y = attackHeadOffsetY_;
        }

        // 非アクティブな首の「棒立ち」を防ぐ（余計なスケール変更はせず震えのみ）
        for (int i = 0; i < 3; ++i) {
            if (i != active) {
                targetScales[i] = {1.0f, 1.0f, 1.0f}; // 判定残存を防ぐため1.0f固定
                targetOffsets[i].x = std::sin(totalTimer_ * 50.0f + i) * 0.15f;
                targetOffsets[i].z = std::cos(totalTimer_ * 40.0f + i) * 0.15f;
                targetOffsets[i].y = idleHeadOffsetY_; // 攻撃していない頭のY位置を下げる
            }
        }
    }

    // 各頭の値をなめらかに適用（非アクティブな頭は自然に元に戻る）
    // 振る速度はSweepフェーズでは速く、それ以外は設定パラメータで追従
    float currentLerpSpeed = (currentPhase_ == AttackPhase::Sweep) ? 0.35f : sweepSpeed_;
    
    auto applyLerp = [&](Vector3& current, const Vector3& target) {
        current = lerpVec3(current, target, currentLerpSpeed);
    };

    applyLerp(enemy->GetHeadLeftOffset(), targetOffsets[0]);
    applyLerp(enemy->GetHeadLeftLocalTransform().rotate, targetRotates[0]);
    applyLerp(enemy->GetHeadLeftLocalTransform().scale, targetScales[0]);

    applyLerp(enemy->GetHeadRightOffset(), targetOffsets[1]);
    applyLerp(enemy->GetHeadRightLocalTransform().rotate, targetRotates[1]);
    applyLerp(enemy->GetHeadRightLocalTransform().scale, targetScales[1]);

    applyLerp(enemy->GetHeadMidOffset(), targetOffsets[2]);
    applyLerp(enemy->GetHeadMidLocalTransform().rotate, targetRotates[2]);
    applyLerp(enemy->GetHeadMidLocalTransform().scale, targetScales[2]);

    // --- ステート遷移 ---
    if (phaseTimer_ >= currentPhaseDuration) {
        phaseTimer_ = 0.0f;
        if (currentPhase_ == AttackPhase::WindUp) {
            currentPhase_ = AttackPhase::Sweep;
        } else if (currentPhase_ == AttackPhase::Sweep) {
            currentPhase_ = AttackPhase::Recovery;
        } else if (currentPhase_ == AttackPhase::Recovery) {
            currentAttackIndex_++;
            if (currentAttackIndex_ > 2) {
                currentPhase_ = AttackPhase::ReturnToIdle;
            } else {
                currentPhase_ = AttackPhase::WindUp;
            }
        } else if (currentPhase_ == AttackPhase::ReturnToIdle) {
            currentPhase_ = AttackPhase::Done;
            hasFinishedAttack_ = true;
            enemy->SetState(EnemyState::Idle);
        }
    }
}

void Phase1_NeckAttack::Exit(Enemy* enemy) {
    // 全オフセット・回転情報をクリーンアップ
    for (int i = 0; i < 3; ++i) {
        enemy->GetBodyOffset(i) = {0,0,0};
    }
    enemy->GetHeadLeftOffset() = {0,0,0};
    enemy->GetHeadRightOffset() = {0,0,0};
    enemy->GetHeadMidOffset() = {0,0,0};

    enemy->GetHeadLeftLocalTransform().rotate = {0,0,0};
    enemy->GetHeadRightLocalTransform().rotate = {0,0,0};
    enemy->GetHeadMidLocalTransform().rotate = {0,0,0};
    
    enemy->GetHeadLeftLocalTransform().scale = {1,1,1};
    enemy->GetHeadRightLocalTransform().scale = {1,1,1};
    enemy->GetHeadMidLocalTransform().scale = {1,1,1};
}
