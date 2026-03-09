#pragma once
#include "Core/math/Vector3.h"

class Enemy;

class EnemyAnimation {
public:
    void Initialize(Enemy* enemy);
    void Update();

private:
    Enemy* enemy_ = nullptr;
    float timer_ = 0.0f;
    float attackTimer_ = 0.0f; // 攻撃フェーズ専用のタイマー
};