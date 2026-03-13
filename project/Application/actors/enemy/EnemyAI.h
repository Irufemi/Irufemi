#pragma once

class Enemy;

class EnemyAI {
public:
    ~EnemyAI();
    void Initialize(Enemy* enemy);
    void Update();

private:
    Enemy* enemy_ = nullptr;
    float timer_ = 0.0f;

    // --- 調整用パラメータ ---
    float stateChangeInterval_ = 15.0f; // 状態を切り替える周期（秒）
    float attackStartTime_ = 5.0f;     // 攻撃に切り替わるタイミング（秒）
};