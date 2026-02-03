#pragma once
#include "contents/wall/Wall.h"
#include "HealerActor.h"

#include <deque>
#include <list>
#include <unordered_map>

#include "math/Vector3.h"
#include "math/Transform.h" // Transform をインクルード

// 前方宣言
class Camera;

// 壊された壁を順番に直す役割
class Healer {
public:
	Healer();
	~Healer();

	// 壊された壁のTransformとサイズを通知する（破壊時に呼び出す）
	void NotifyWallDestroyed(const Transform& transform, const Vector3& wallSize);

	// 毎フレーム呼び出す。修復可能なときに次の壊れた壁を復元する。
	// camera は Wall の初期化に必要。walls は復元先のコンテナ（nullptr のスロットを探す）
	void Update(Camera* camera, std::list<Wall*>& walls, std::list<HealerActor*>& healers);

private:
	struct DestroyedWallInfo {
		Transform transform; // 位置と回転
		Vector3 wallSize;    // 壁のサイズ
		std::vector<HealerActor*> assignedHealers; // 演出用に割り当てられたHealerActor複数
		float progress = 0.0f; // 修復進捗（フレーム換算）
	};

	// 壁位置をキー化する(簡易ハッシュ用)
	static size_t MakeWallKey(const Transform& t);

	std::deque<DestroyedWallInfo> destroyedQueue_{};
	// 各エントリごとに進捗を持つため、グローバルな healFrameCounter_ は不要
	static inline const int kHealIntervalFrames = 180; // 180フレームごとに1つ修復

	// 復活回数の管理(位置キー→回数)
	std::unordered_map<size_t, int> reviveCounts_{};
	static inline const int kMaxRevivesPerWall = 2;
};