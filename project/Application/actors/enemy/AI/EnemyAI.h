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
    float attackInterval_ = 8.0f;  // 攻撃終了から次の攻撃までの秒数
    float startDelay_ = 5.0f;      // ゲーム開始から最初の攻撃までの秒数（ここを調整）
    bool isFirstAttackStarted_ = false; // 最初の攻撃を開始したか
    bool nextIsStomp_ = false; // どっちの攻撃をするかのフラグ
};