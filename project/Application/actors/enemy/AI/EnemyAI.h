#pragma once

#include <random>

class Enemy;
class Player;

class EnemyAI {
public:
    ~EnemyAI();
    void Initialize(Enemy* enemy);
    void Update(Player* player, float deltaTime);

private:
    const float kZeroThreshold = 0.0f;

    Enemy* enemy_ = nullptr;
    float timer_ = 0.0f;           // 汎用タイマー
    float attackWaitTimer_ = 0.0f; // 攻撃終了後の待機用タイマー
    bool isWaitingForNextAttack_ = false; // 待機中かどうかのフラグ

    // --- 調整用パラメータ ---
    float attackIntervalBase_ = 7.0f;     // 攻撃終了から次の攻撃までの基準秒数
    float attackIntervalVariance_ = 3.0f; // 攻撃間隔のブレ幅（±この値）
    float currentAttackInterval_ = 8.0f;  // 現在設定されている攻撃間隔

    float startDelay_ = 5.0f;      // ゲーム開始から最初の攻撃までの秒数（ここを調整）
    bool isFirstAttackStarted_ = false; // 最初の攻撃を開始したか
    
    int previousAttackIndex_ = -1; // 前回の攻撃（連続で同じ攻撃をしないため）

    // 距離の閾値
    float closeRangeThreshold_ = 50.0f; // 近距離の閾値
    float midRangeThreshold_ = 125.0f;   // 中距離の閾値

    // 乱数生成器
    std::mt19937 randomEngine_;
};