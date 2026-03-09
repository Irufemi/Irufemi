#pragma once
class Enemy;

class EnemyAI {
public:
	~EnemyAI();
	void Initialize(Enemy* enemy);
	void Update();

private:
	Enemy* enemy_ = nullptr;
	float timer_ = 0.0f; // アニメーション・行動用タイマー
};