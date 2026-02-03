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

// 壊れた壁を優先的に修復するための距離2計算(既存と共有)
static float DistanceSq(const Vector3& a, const Vector3& b) {
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float dz = a.z - b.z;
	return dx * dx + dy * dy + dz * dz;
}

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
	// キューが空だったかを確認（空なら最初の破壊通知）
	bool wasEmpty = destroyedQueue_.empty();
	// 復活予定としてキューに積む
	destroyedQueue_.push_back(DestroyedWallInfo{ transform, wallSize, {}, 0.0f });
	// 既に修復キューがある状態で新たな壊れた壁が来た場合、
	// タイマーをリセットせずに引き継ぐ（要求どおり）。
	if (wasEmpty) {
		// 最初の要素が追加されたタイミングで進捗を0にしておくことで
		// 初回復元までのカウントを開始する。
		destroyedQueue_.front().progress = 0.0f;
	}
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

	// 壊れた壁があればヒーラーを割り当てる
	if (!destroyedQueue_.empty()) {
		// ポインタの配列を作成してソート（deque を直接インデックスで参照すると破壊と競合する可能性があるため）
		std::vector<DestroyedWallInfo*> ptrs;
		ptrs.reserve(destroyedQueue_.size());
		for (auto &entry : destroyedQueue_) ptrs.push_back(&entry);
		std::sort(ptrs.begin(), ptrs.end(), [&](DestroyedWallInfo* a, DestroyedWallInfo* b) {
			const Vector3 origin{ 0.0f, 0.0f, 0.0f };
			return DistanceSq(a->transform.translate, origin) > DistanceSq(b->transform.translate, origin); // 外側優先
		});

		for (DestroyedWallInfo* pInfo : ptrs) {
			DestroyedWallInfo& info = *pInfo;
			int need = kMaxPerWall - static_cast<int>(info.assignedHealers.size());
			if (need <= 0) continue;

			while (need > 0) {
				if (availableHealers.empty()) {
					HealerActor* newHa = new HealerActor();
					Vector3 spawnOffset;
					spawnOffset.x = Random::GeneratorFloat(-info.wallSize.x, info.wallSize.x) * 0.5f;
					spawnOffset.y = Random::GeneratorFloat(-info.wallSize.y, info.wallSize.y) * 0.5f;
					spawnOffset.z = 0.0f;
					Vector3 spawnPos = { info.transform.translate.x + spawnOffset.x, info.transform.translate.y + spawnOffset.y, info.transform.translate.z + spawnOffset.z };
					newHa->Initialize(camera, spawnPos);
					healers.push_back(newHa);
					availableHealers.push_back(newHa);
				}

				if (availableHealers.empty()) break;

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

				Vector3 randomOffset;
				randomOffset.x = Random::GeneratorFloat(-info.wallSize.x / 2.0f, info.wallSize.x / 2.0f);
				randomOffset.y = Random::GeneratorFloat(-info.wallSize.y / 2.0f, info.wallSize.y / 2.0f);
				randomOffset.z = 0.0f;

				Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(info.transform.rotate.x, info.transform.rotate.y, info.transform.rotate.z);
				Vector3 worldOffset = Math::Transform(randomOffset, rotMat);

				Vector3 targetPos = info.transform.translate + worldOffset;
				targetPos.z = info.transform.translate.z;
				chosen->SetTargetPosition(targetPos);

				availableHealers.erase(availableHealers.begin() + bestIdx);
				--need;
			}
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

	// 壁ごとの進捗を進める。各順位は 0.5 の累乗で速度低下（1.0, 0.5, 0.25, 0.125...）
	if (!destroyedQueue_.empty()) {
		std::vector<DestroyedWallInfo*> ptrs;
		ptrs.reserve(destroyedQueue_.size());
		for (auto &entry : destroyedQueue_) ptrs.push_back(&entry);
		std::sort(ptrs.begin(), ptrs.end(), [&](DestroyedWallInfo* a, DestroyedWallInfo* b) {
			const Vector3 origin{0.0f, 0.0f, 0.0f};
			return DistanceSq(a->transform.translate, origin) > DistanceSq(b->transform.translate, origin);
		});

		for (size_t order = 0; order < ptrs.size(); ++order) {
			DestroyedWallInfo* pInfo = ptrs[order];
			float speedMultiplier = std::pow(0.5f, static_cast<float>(order));
			pInfo->progress += speedMultiplier;
			if (pInfo->progress >= static_cast<float>(kHealIntervalFrames)) {
				// nullptr スロットを探して復元
				for (Wall*& w : walls) {
					if (w == nullptr) {
						Wall* newWall = new Wall();
						const DestroyedWallInfo infoCopy = *pInfo; // copy for use after erase

						// 復活回数を増加
						size_t key = MakeWallKey(infoCopy.transform);
						int& cnt = reviveCounts_[key];
						cnt = cnt + 1;

						std::string modelFilename = "TD_DamageBlock.obj";
						if (cnt >= 2) modelFilename = "TD_OverDamageBlock.obj";

						Vector3 restoredPos = infoCopy.transform.translate;
						restoredPos.z = 0.0f;
						newWall->Initialize(camera, restoredPos, modelFilename);
						newWall->SetRotation(infoCopy.transform.rotate);
						newWall->SetScale(infoCopy.transform.scale);
						newWall->Update();
						w = newWall;

						w->StartRepairAnimation();

						// 割り当てられていた HealerActor を削除
						for (HealerActor* assigned : infoCopy.assignedHealers) {
							if (!assigned) continue;
							for (HealerActor*& slot : healers) {
								if (slot == assigned) {
									delete slot;
									slot = nullptr;
									break;
								}
							}
						}

						// deque から該当要素を削除（ポインタ比較）
						for (auto it = destroyedQueue_.begin(); it != destroyedQueue_.end(); ++it) {
							if (&(*it) == pInfo) {
								destroyedQueue_.erase(it);
								break;
							}
						}

						break; // walls ループを抜ける
					}
				}
			}
		}
	}
}
