#pragma once
#include "../wall/Wall.h"
#include "HealerActor.h"

#include <deque>
#include <list>

#include "math/Vector3.h"

// 前方宣言
class Camera;

// 壊された壁を順番に直す役割
class Healer {
public:
	Healer();
	~Healer();

	// 壊された壁の位置と回転を通知する（破壊時に呼び出す）
	void NotifyWallDestroyed(const Vector3& pos, const Vector3& rot);

	// 毎フレーム呼び出す。修復可能なときに次の壊れた壁を復元する。
	// camera は Wall の初期化に必要。walls は復元先のコンテナ（nullptr のスロットを探す）
	void Update(Camera* camera, std::list<Wall*>& walls, std::list<HealerActor*>& healers);

private:
	struct DestroyedWallInfo {
		Vector3 pos;
		Vector3 rot;
		std::vector<HealerActor*> assignedHealers; // 演出用に割り当てられたHealerActor複数
	};

	std::deque<DestroyedWallInfo> destroyedQueue_{};
	int healFrameCounter_ = 0;
	static inline const int kHealIntervalFrames = 180; // 180フレームごとに1つ修復
};