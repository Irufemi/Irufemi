#pragma once

class Enemy;

class EnemyAI {
public:
  ~EnemyAI();

  void Initialize(Enemy* enemy);
  void Update();

private:
  Enemy* enemy_ = nullptr;
  
  // アニメーション用タイマーなどAI専用の変数をここに持たせられます
  float timer_ = 0.0f;
};
