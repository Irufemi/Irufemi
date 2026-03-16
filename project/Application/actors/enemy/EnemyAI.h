#pragma once

class Enemy;

class EnemyAI {
public:
    ~EnemyAI();
    void Initialize(Enemy* enemy);
    void Update();

private:
    Enemy* enemy_ = nullptr;
    float timer_ = 0.0f;           // 汎用タイマー
    float attackWaitTimer_ = 0.0f; // 攻撃終了後の待機用タイマー
    bool isWaitingForNextAttack_ = false; // 待機中かどうかのフラグ

    // --- 調整用パラメータ ---
    float attackInterval_ = 5.0f;  // 攻撃終了から次の攻撃までの秒数
};