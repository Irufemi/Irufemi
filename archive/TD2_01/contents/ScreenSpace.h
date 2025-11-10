#pragma once

// 依存を最小にするため前方宣言のみ
struct Vector2;
struct Vector3;
class  Camera;

// スクリーン座標 (x,y,[0..1 depth]) を Z=targetZ のワールド平面へ射影してワールド座標を返す
[[nodiscard]] Vector3 ScreenToWorldOnZ(const Camera* cam, const Vector2& screen, float targetZ);

// 画面上の半径[pixels]を、Z=targetZ 平面上のワールド半径へ変換
[[nodiscard]] float ScreenRadiusToWorld(const Camera* cam, const Vector2& center, float radiusPx, float targetZ);