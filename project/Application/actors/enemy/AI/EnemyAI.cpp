#include "EnemyAI.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "Core/Math/Math.h"
#include <cmath>

EnemyAI::~EnemyAI() {}

void EnemyAI::Initialize(Enemy* enemy) {
    enemy_ = enemy;
    timer_ = kZeroThreshold;
    attackWaitTimer_ = kZeroThreshold;
    isWaitingForNextAttack_ = false;
    isFirstAttackStarted_ = false; // 初期化
    previousAttackIndex_ = -1;
    currentAttackInterval_ = attackIntervalBase_;

    // 乱数生成器の初期化
    std::random_device rd;
    randomEngine_.seed(rd());
}

void EnemyAI::Update(Player* player, float deltaTime) {
    if (!enemy_) return;

    // フェーズ2中は、個別の首が自律行動するため既存のAIルーチンを停止する
    if (enemy_->GetState() == EnemyState::Phase2) {
        return;
    }

    // 1. 最初の待機時間を消化
    if (!isFirstAttackStarted_) {
        timer_ += deltaTime;
        if (timer_ >= startDelay_) {
            isFirstAttackStarted_ = true;
            enemy_->SetState(EnemyState::Attack_Tackle);
            previousAttackIndex_ = 3; // 最初の攻撃はTackle(3)
        }
        return;
    }

    // 2. 次の攻撃までの待機処理 (待機フラグが立っている間)
    if (isWaitingForNextAttack_) {
        attackWaitTimer_ += deltaTime;
        if (attackWaitTimer_ >= currentAttackInterval_) {
            isWaitingForNextAttack_ = false;

            // プレイヤーとの距離を計算
            float distanceToPlayer = 0.0f;
            if (player) {
                Vector3 diff = Math::Subtract(player->GetTranslate(), enemy_->GetGlobalTransform().translate);
                distanceToPlayer = Math::Length(diff);
            }

            int nextAttack = -1;
            std::uniform_real_distribution<float> randFloat(0.0f, 1.0f);
            float randVal = randFloat(randomEngine_);

            // 距離に応じた行動選択の重み付け
            if (distanceToPlayer < closeRangeThreshold_) {
                // 近距離: ストンプ(1) または ネック攻撃(2) を優先
                if (randVal < 0.6f && previousAttackIndex_ != 1) {
                    nextAttack = 1; // Stomp
                } else if (previousAttackIndex_ != 2) {
                    nextAttack = 2; // Neck
                } else {
                    nextAttack = 1; // 連続になる場合はStompを許容
                }
            } else if (distanceToPlayer < midRangeThreshold_) {
                // 中距離: ネック(2) または タックル(3) または ビーム(0)
                if (randVal < 0.4f && previousAttackIndex_ != 2) {
                    nextAttack = 2; // Neck
                } else if (randVal < 0.7f && previousAttackIndex_ != 3) {
                    nextAttack = 3; // Tackle
                } else {
                    nextAttack = 0; // Beam
                }
            } else {
                // 遠距離: ビーム(0) または タックル(3) を優先
                if (randVal < 0.6f && previousAttackIndex_ != 0) {
                    nextAttack = 0; // Beam
                } else if (previousAttackIndex_ != 3) {
                    nextAttack = 3; // Tackle
                } else {
                    nextAttack = 0; // 連続になる場合はBeamを許容
                }
            }

            // 安全策：nextAttackが決まっていない場合
            if (nextAttack == -1) {
                std::uniform_int_distribution<int> randInt(0, 3);
                nextAttack = randInt(randomEngine_);
                while (nextAttack == previousAttackIndex_) {
                    nextAttack = randInt(randomEngine_);
                }
            }

            // 行動の実行
            if (nextAttack == 0) {
                enemy_->SetState(EnemyState::Attack_Beam);
            } else if (nextAttack == 1) {
                enemy_->SetState(EnemyState::Attack_Stomp);
            } else if (nextAttack == 2) {
                enemy_->SetState(EnemyState::Attack_Neck);
            } else if (nextAttack == 3) {
                enemy_->SetState(EnemyState::Attack_Tackle);
            }

            previousAttackIndex_ = nextAttack;
        }

    // 3. 攻撃中：アニメーションが完了したか監視
    } else {
        if (enemy_->GetAnimation()->HasFinishedAttack()) {
            enemy_->SetState(EnemyState::Idle);
            isWaitingForNextAttack_ = true;
            attackWaitTimer_ = kZeroThreshold;
            
            // 次の攻撃間隔をランダムに決定
            std::uniform_real_distribution<float> intervalDist(-attackIntervalVariance_, attackIntervalVariance_);
            currentAttackInterval_ = attackIntervalBase_ + intervalDist(randomEngine_);
            if (currentAttackInterval_ < 1.0f) {
                currentAttackInterval_ = 1.0f; // 最低でも1秒は待つ
            }
        }
    }
}