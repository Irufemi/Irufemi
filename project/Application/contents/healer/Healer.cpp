#include "Healer.h"

#include "camera/Camera.h"

Healer::Healer() {}

Healer::~Healer() {}

void Healer::NotifyWallDestroyed(const Vector3& pos, const Vector3& rot) {
	destroyedQueue_.push(DestroyedWallInfo{pos, rot});
}

void Healer::Update(Camera* camera, std::list<Wall*>& walls) {
	if (destroyedQueue_.empty()) return;

	++healFrameCounter_;
	if (healFrameCounter_ < kHealIntervalFrames) return;

	healFrameCounter_ = 0;

	// 復元先として nullptr のスロットを探す。見つかればそこで壁を復元する。
	for (Wall*& w : walls) {
		if (w == nullptr) {
			Wall* newWall = new Wall();
			const DestroyedWallInfo info = destroyedQueue_.front();
			newWall->Initialize(camera, info.pos);
			newWall->SetRotation(info.rot);
			newWall->Update();
			w = newWall;
			destroyedQueue_.pop();
			break;
		}
	}
}
