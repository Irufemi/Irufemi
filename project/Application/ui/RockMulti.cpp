#include "RockMulti.h"

#include "camera/Camera.h"
#include "engine/IrufemiEngine.h"

void RockMulti::Initialize(IrufemiEngine *engine, Camera *camera) {
  engine_ = engine;
  camera_ = camera;

  // X のスプライト初期化
  xSprite_.Initialize(camera_, "resources/X.png");

  // 数字用のテキスト初期化
  numSprite_.Initialize(camera_, "resources/number.png");
}

void RockMulti::Show(int multiplier, const Vector3 &worldPos) {
  active_ = true;
  multiplier_ = multiplier;
  worldPos_ = worldPos;
  timer_ = 0.0f;
}

void RockMulti::Update(float deltaTime) {
  if (!active_) {
    return;
  }

  timer_ += deltaTime;
  if (timer_ >= lifeTime_) {
    active_ = false;
    return;
  }

  // 少し上に移動させる
  worldPos_.y += floatSpeed_ * deltaTime;

  numSprite_.Update();

  xSprite_.Update();
}

void RockMulti::Draw() {
  if (!active_) {
    return;
  }

  // TODO: Camera に合わせて worldPos_ -> 画面座標に変換する
  // 例: camera_->WorldToScreen(worldPos_) 的な関数を用意する or
  //     GameScene 側でスクリーン座標を渡すようにしてもいい

  Vector2 screenPos = camera_->WorldToScreen(worldPos_);

  // フェードアウト用アルファ
  float t = timer_ / lifeTime_;
  if (t > 1.0f)
    t = 1.0f;
  float alpha = 1.0f - t;

  // X の描画
  xSprite_.SetPosition(screenPos.x, screenPos.y);
  xSprite_.SetColor({1.0f, 1.0f, 1.0f, alpha});
  xSprite_.Draw();

  // 数字部分の描画
  // X の右側に少しずらして描く
  Vector2 numPos = screenPos;
  numPos.x += 32.0f;
  numPos.y -= 18.0f;

  numSprite_.SetPosition(numPos.x, numPos.y);
  numSprite_.SetTextureRectPixels((multiplier_) * 32, 0, 32, 64);
  numSprite_.SetSize(64.0f,64.0f);
  numSprite_.SetColor({1.0f, 1.0f, 1.0f, alpha});
  numSprite_.Draw();
}