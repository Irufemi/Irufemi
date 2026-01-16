#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "math/Vector3.h"

// マップチップの種類
enum MapChipType { kBlank, kBlock };

// CSV 読み込み後の 2D 配列
struct MapChipData {
	std::vector<std::vector<MapChipType>> data;
};

/// <summary>
/// マップチップフィールド
/// ※ 座標系：x 右+, y 上+。タイル原点は中心。
/// </summary>
class MapChipField {
public: // ===== Public types =====
	struct IndexSet {
		int xIndex;
		int yIndex;
	};
	struct Rect {
		float left;
		float right;
		float bottom;
		float top;
	};

public:                                               // ===== Public API =====
	void ResetMapChipData();                          // 配列リサイズ＆初期化
	void LoadMapChipCsv(const std::string& filePath); // CSV を data へロード

	// ★ インデックス/座標←→相互変換とタイル情報取得（範囲外は安全に空扱い）
	MapChipType GetMapChipTypeByIndex(int xIndex, int yIndex);
	Vector3 GetMapChipPositionByIndex(int xIndex, int yIndex);      // タイル中心座標
	IndexSet GetMapChipIndexSetByPosition(const Vector3& position); // 位置→インデックス
	Rect GetRectByIndex(int xIndex, int yIndex);                                  // タイル AABB

	// フィールドサイズ（タイル数）
	uint32_t GetNumBlockVirtical() { return kNumBlockVirtical; }
	uint32_t GetNumBlockHorizontal() { return kNumBlockHorizontal; }

private: // ===== Data & constants =====
	// タイル寸法・フィールドタイル数
	static inline const float kBlockWidth = 1.0f;
	static inline const float kBlockHeight = 1.0f;
	static inline const uint32_t kNumBlockVirtical = 20;
	static inline const uint32_t kNumBlockHorizontal = 100;

	MapChipData mapChipData_{};
};
