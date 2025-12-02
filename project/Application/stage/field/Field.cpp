#include "Field.h"
#include <cmath>
#include <cstdlib>
#include "manager/DebugUI.h"

void Field::ClampInside(Vector3& pos, float objRadius) const {

	// フィールド中心からのXZ方向ベクトル
	Vector3 diff{
		pos.x - center.x,
		0.0f,
		pos.z - center.z
	};

	float distSq = diff.x * diff.x + diff.z * diff.z;

	// 許される最大距離（フィールド半径 − オブジェクト半径）
	float maxDist = radius - objRadius;
	if (maxDist < 0.0f) {
		maxDist = 0.0f;
	}
	float maxDistSq = maxDist * maxDist;

	// まだ円の内側ならOK
	if (distSq <= maxDistSq) {
		return;
	}

	// 円の外に出たので円周まで押し戻す
	float dist = std::sqrt(distSq);

	// ど真ん中にいて diff が0の場合の保険
	if (dist == 0.0f) {
		diff.x = 1.0f;
		diff.z = 0.0f;
		dist = 1.0f;
	}

	// 正規化
	diff.x /= dist;
	diff.z /= dist;

	// 円周の位置
	pos.x = center.x + diff.x * maxDist;
	pos.z = center.z + diff.z * maxDist;
}

Vector3 Field::GetRandomPointInside(float y) const {

	// 0〜1 の乱数（float）
	float u = (float)rand() / RAND_MAX;
	float v = (float)rand() / RAND_MAX;

	// 一様分布になるように平方根を使う
	float r = std::sqrt(u) * radius;
	float rad = v * 2.0f * 3.1415926535f;

	float x = center.x + r * std::cos(rad);
	float z = center.z + r * std::sin(rad);

	return Vector3{ x, y, z };
}

// === imguiで編集 ===
void Field::DrawImGui() {
	if (ImGui::Begin("Field Debug")) {
		ImGui::DragFloat("Center X", &center.x, 0.1f);
		ImGui::DragFloat("Center Z", &center.z, 0.1f);
		ImGui::DragFloat("Radius", &radius, 0.1f, 0.0f, 200.0f);

		ImGui::Separator();
		ImGui::Text("Now: Center(%.2f, %.2f, %.2f)", center.x, center.y, center.z);
		ImGui::Text("Radius: %.2f", radius);
	}
	ImGui::End();
}