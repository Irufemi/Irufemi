#pragma once

#include <memory>
#include <vector>
#include <list>
#include "math/Vector3.h"

// 前方宣言
class Sprite;
class Camera;
class Enemy;
class IrufemiEngine;

// 画面外の敵の位置を示すインジケーター
class OffScreenIndicator {
public:
	OffScreenIndicator();
	~OffScreenIndicator();

	void Initialize(Camera* camera, IrufemiEngine* engine);
	void Update(const std::list<Enemy*>& attackingEnemies);
	void Draw();

private:
	struct Indicator {
		std::unique_ptr<Sprite> sprite;
		const Enemy* targetEnemy = nullptr;
	};

	Camera* camera_ = nullptr;
	IrufemiEngine* engine_ = nullptr;
	std::vector<Indicator> indicators_;
	static const int kMaxIndicators_ = 10; // 同時に表示するインジケーターの最大数
};