#include "MapChipField.h"

#include <cassert>
#include <fstream>
#include <map>
#include <sstream>

namespace {
// CSV の文字→種類の対応表
std::map<std::string, MapChipType> mapChipTable = {
    {"0", MapChipType::kBlank},
    {"1", MapChipType::kBlock},
};
} // namespace

void MapChipField::ResetMapChipData() {
	// 2D 配列をフィールドサイズに合わせて確保
	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVirtical);
	for (auto& line : mapChipData_.data) {
		line.resize(kNumBlockHorizontal);
	}
}

void MapChipField::LoadMapChipCsv(const std::string& filePath) {
	// CSV を読み込んで mapChipData_.data に展開
	ResetMapChipData();
	std::ifstream file(filePath);
	assert(file.is_open());
	std::stringstream mapChipCsv;
	mapChipCsv << file.rdbuf();
	file.close();
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		std::string line;
		getline(mapChipCsv, line);
		std::istringstream line_stream(line);
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			std::string word;
			getline(line_stream, word, ',');
			if (mapChipTable.contains(word)) {
				mapChipData_.data[i][j] = mapChipTable[word];
			}
		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(int xIndex, int yIndex) {
	// 範囲外アクセスは安全に空タイルとして扱う
	if (xIndex < 0 || kNumBlockHorizontal - 1 < xIndex) {
		return MapChipType::kBlank;
	}
	if (yIndex < 0 || kNumBlockVirtical - 1 < yIndex) {
		return MapChipType::kBlank;
	}
	return mapChipData_.data[yIndex][xIndex];
}

Vector3 MapChipField::GetMapChipPositionByIndex(int xIndex, int yIndex) {
	// タイル中心座標。y は「上 +」の画面系なので配列インデックスを反転
	return Vector3(kBlockWidth * xIndex, kBlockHeight * (kNumBlockVirtical - 1 - yIndex), 0);
}

MapChipField::IndexSet MapChipField::GetMapChipIndexSetByPosition(const Vector3& position) {
	// 位置→インデックス（中心基準）。y は配列行に合わせて反転
	IndexSet indexSet{};
	indexSet.xIndex = static_cast<int>((position.x + kBlockWidth / 2.0f) / kBlockWidth);
	indexSet.yIndex = static_cast<int>((position.y + kBlockHeight / 2.0f) / kBlockHeight);
	indexSet.yIndex = kNumBlockVirtical - 1 - indexSet.yIndex; // y 軸は上が + のため補正
	return indexSet;
}

MapChipField::Rect MapChipField::GetRectByIndex(int xIndex, int yIndex) {
	// タイルの AABB を返す（中心座標 ± 半サイズ）
	Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);
	Rect rect{};
	rect.left = center.x - kBlockWidth / 2.0f;
	rect.right = center.x + kBlockWidth / 2.0f;
	rect.bottom = center.y - kBlockHeight / 2.0f;
	rect.top = center.y + kBlockHeight / 2.0f;
	return rect;
}