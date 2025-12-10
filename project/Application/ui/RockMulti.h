#pragma once
#include "2D/Sprite.h"
#include "math/Vector3.h"

class IrufemiEngine;
class Camera;

class RockMulti {
public:
  RockMulti() = default;

  // エンジンとカメラを受け取って初期化
  void Initialize(IrufemiEngine *engine, Camera *camera);

  // 倍率アップ時に呼ぶ。worldPos はプレイヤーの位置とか
  void Show(int multiplier, const Vector3 &worldPos);

  // 毎フレーム更新
  void Update(float deltaTime);

  // 描画
  void Draw();

  bool IsActive() const { return active_; }

private:
  IrufemiEngine *engine_ = nullptr;
  Camera *camera_ = nullptr;

  // Xの画像
  Sprite xSprite_;
  // 数字部分
  Sprite numSprite_;

  bool active_ = false;
  int multiplier_ = 1;

  // 出すワールド座標
  Vector3 worldPos_{};

  // 演出用タイマー
  float timer_ = 0.0f;
  float lifeTime_ = 1.5f;

  // 少し上にふわっと動かす用
  float floatSpeed_ = 1.0f;
};
