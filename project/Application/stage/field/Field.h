#pragma once
#include "math/Vector3.h"

class Field {
public:
	// フィールド中心（XZ平面）。Yは基本無視。
	Vector3 center;

	// フィールドの半径
	float radius = 10.0f;


public:
	// プレイヤー（または任意の円）をフィールド内に収める
	void ClampInside(Vector3& pos, float objRadius) const;

	// 後で必要になったら使える：円の中からランダム取得
	Vector3 GetRandomPointInside(float y = 0.0f) const;

	//追加：ImGui デバッグ用
	void DrawImGui();
};
