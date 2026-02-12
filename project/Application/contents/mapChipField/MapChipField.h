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

/**
 * @class MapChipField
 * @brief CSVファイルから読み込んだデータに基づいてマップチップを管理するクラス
 *
 * 座標系：x 右+, y 上+。タイル原点は中心。
 */
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
	/**
	 * @brief マップチップデータをリセットし、配列を再確保します
	 */
	void ResetMapChipData();
	/**
	 * @brief 指定されたCSVファイルからマップデータを読み込みます
	 * @param filePath CSVファイルのパス
	 */
	void LoadMapChipCsv(const std::string& filePath);

	/**
	 * @brief 指定したインデックスのマップチップ種類を取得します
	 * @param xIndex X方向のインデックス
	 * @param yIndex Y方向のインデックス
	 * @return MapChipType マップチップの種類
	 */
	MapChipType GetMapChipTypeByIndex(int xIndex, int yIndex) const;
	/**
	 * @brief 指定したインデックスのマップチップの中心座標を取得します
	 * @param xIndex X方向のインデックス
	 * @param yIndex Y方向のインデックス
	 * @return Vector3 ワールド座標
	 */
	Vector3 GetMapChipPositionByIndex(int xIndex, int yIndex) const;
	/**
	 * @brief ワールド座標から対応するマップチップのインデックスを取得します
	 * @param position ワールド座標
	 * @return IndexSet マップチップのインデックス
	 */
	IndexSet GetMapChipIndexSetByPosition(const Vector3& position) const;
	/**
	 * @brief 指定したインデックスのマップチップの矩形（AABB）を取得します
	 * @param xIndex X方向のインデックス
	 * @param yIndex Y方向のインデックス
	 * @return Rect 矩形情報
	 */
	Rect GetRectByIndex(const int& xIndex, const int& yIndex) const;

	/**
	 * @brief マップの垂直方向のタイル数を取得します
	 * @return uint32_t 垂直方向のタイル数
	 */
	uint32_t GetNumBlockVirtical() const { return kNumBlockVirtical; }
	/**
	 * @brief マップの水平方向のタイル数を取得します
	 * @return uint32_t 水平方向のタイル数
	 */
	uint32_t GetNumBlockHorizontal() const { return kNumBlockHorizontal; }

private: // ===== Data & constants =====
	// タイル寸法・フィールドタイル数
	static inline const float kBlockWidth = 1.0f;
	static inline const float kBlockHeight = 1.0f;
	static inline const uint32_t kNumBlockVirtical = 20;
	static inline const uint32_t kNumBlockHorizontal = 100;

	MapChipData mapChipData_;
};
