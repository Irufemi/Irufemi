#include "Healer.h"

#include <cfloat>
#include <cmath>
#include <vector>
#include <algorithm>

#include "camera/Camera.h"
#include "function/Random.h"
#include "function/Math.h"

Healer::Healer() {}

Healer::~Healer() {}

// 壁位置キー生成(位置と回転を丸めてハッシュ)
size_t Healer::MakeWallKey(const Transform& t) {
	auto quantize = [](float v) {
		return static_cast<int>(std::round(v * 100.0f)); // 0.01 単位で丸め
	};
	size_t seed = 0;
	auto hashCombine = [&seed](size_t h) { seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2); };
	hashCombine(std::hash<int>{}(quantize(t.translate.x)));
	hashCombine(std::hash<int>{}(quantize(t.translate.y)));
	hashCombine(std::hash<int>{}(quantize(t.translate.z)));
	hashCombine(std::hash<int>{}(quantize(t.rotate.x)));
	hashCombine(std::hash<int>{}(quantize(t.rotate.y)));
	hashCombine(std::hash<int>{}(quantize(t.rotate.z)));
	return seed;
}

void Healer::NotifyWallDestroyed(const Transform& transform, const Vector3& wallSize) {
	// 復活上限チェック
	size_t key = MakeWallKey(transform);
	int count = 0;
	if (auto it = reviveCounts_.find(key); it != reviveCounts_.end()) {
		count = it->second;
	}
	if (count >= kMaxRevivesPerWall) {
		// 上限に達しているので復活キューに入れない
		return;
	}
	// 復活予定としてキューに積む
	destroyedQueue_.push_back(DestroyedWallInfo{ transform, wallSize, {} });
	healFrameCounter_ = 0;
}

static float DistanceSq(const Vector3& a, const Vector3& b) {
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float dz = a.z - b.z;
	return dx * dx + dy * dy + dz * dz;
}

void Healer::Update(Camera* camera, std::list<Wall*>& walls, std::list<HealerActor*>& healers) {
	// 最大同一壁あたりの割り当て数
	static const int kMaxPerWall = 5;


	std::vector<HealerActor*> availableHealers;
	availableHealers.reserve(healers.size());
	for (HealerActor* ha : healers) {
		if (!ha) continue;
		if (ha->IsAssigned()) continue;
		availableHealers.push_back(ha);
	}


	for (DestroyedWallInfo& info : destroyedQueue_) {
		int need = kMaxPerWall - static_cast<int>(info.assignedHealers.size());
		if (need <= 0) continue;

		while (need > 0) {
			// 利用可能なヒーラーがいない場合は、新しいヒーラーを生成してヒーラーリストに追加
			if (availableHealers.empty()) {
				// 壁の破壊位置周辺に新しいHealerActorを生成
				HealerActor* newHa = new HealerActor();
				// 壁から少しオフセットされた位置に出現させる
				Vector3 spawnOffset;
				spawnOffset.x = Random::GeneratorFloat(-info.wallSize.x, info.wallSize.x) * 0.5f;
				spawnOffset.y = Random::GeneratorFloat(-info.wallSize.y, info.wallSize.y) * 0.5f;
				spawnOffset.z = 0.0f;
				Vector3 spawnPos = { info.transform.translate.x + spawnOffset.x, info.transform.translate.y + spawnOffset.y, info.transform.translate.z + spawnOffset.z };
				newHa->Initialize(camera, spawnPos);
				// 外部コンテナに追加して、GameSceneがそのライフタイムを管理できるようにする
				healers.push_back(newHa);
				availableHealers.push_back(newHa);
			}

			if (availableHealers.empty()) break; // セーフティチェック

			int bestIdx = -1;
			float bestDist = FLT_MAX;
			for (int i = 0; i < (int)availableHealers.size(); ++i) {
				HealerActor* ha = availableHealers[i];
				float d = DistanceSq(ha->GetPosition(), info.transform.translate);
				if (d < bestDist) { bestDist = d; bestIdx = i; }
			}
			if (bestIdx < 0) break;
			HealerActor* chosen = availableHealers[bestIdx];
			info.assignedHealers.push_back(chosen);
			chosen->SetAssigned(true);

			// Wallのローカル空間でランダムなオフセットを生成
			Vector3 randomOffset;
			randomOffset.x = Random::GeneratorFloat(-info.wallSize.x / 2.0f, info.wallSize.x / 2.0f);
			randomOffset.y = Random::GeneratorFloat(-info.wallSize.y / 2.0f, info.wallSize.y / 2.0f);
			randomOffset.z = Random::GeneratorFloat(-info.wallSize.z / 2.0f, info.wallSize.z / 2.0f);

			// オフセットをワールド空間に変換
			Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(info.transform.rotate.x, info.transform.rotate.y, info.transform.rotate.z);
			Vector3 worldOffset = Math::Transform(randomOffset, rotMat);

			// 目標位置を設定
			chosen->SetTargetPosition(info.transform.translate + worldOffset);


			availableHealers.erase(availableHealers.begin() + bestIdx);
			--need;
		}
	}


	const float speed = 0.1f;
	for (DestroyedWallInfo& info : destroyedQueue_) {
		for (HealerActor* ha : info.assignedHealers) {
			if (!ha) continue;
			ha->MoveTowards(ha->GetTargetPosition(), speed, walls);
			ha->RefreshTransform();
		}
	}

	++healFrameCounter_;
	if (healFrameCounter_ < kHealIntervalFrames) return;

	healFrameCounter_ = 0;

	if (destroyedQueue_.empty()) return;

	// 復元先として nullptr のスロットを探す。見つかればそこで壁を復元する。
	for (Wall*& w : walls) {
		if (w == nullptr) {
			Wall* newWall = new Wall();
			const DestroyedWallInfo info = destroyedQueue_.front();
			
			// Transformから位置と回転を復元
			newWall->Initialize(camera, info.transform.translate, "TD_DamageBlock.obj");
			newWall->SetRotation(info.transform.rotate);
			newWall->Update();
			w = newWall;

			// 修復演出
			w->StartRepairAnimation();

			// 復活回数を増加
			size_t key = MakeWallKey(info.transform);
			int& cnt = reviveCounts_[key];
			cnt = cnt + 1; // 初回なら 0→1

			// 割り当てられていた HealerActor がいれば削除してスロットを nullptr にする
			for (HealerActor* assigned : info.assignedHealers) {
				if (!assigned) continue;
				for (HealerActor*& slot : healers) {
					if (slot == assigned) {
						delete slot;
						slot = nullptr;
						break;
					}
				}
			}

			destroyedQueue_.pop_front();
			break;
		}
	}
}
