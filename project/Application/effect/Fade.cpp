#include "Fade.h"
#include "engine/IrufemiEngine.h"

void Fade::Initialize(IrufemiEngine *engine, Camera *camera) {
  engine_ = engine;

  sprite_ = std::make_unique<Sprite>();
  // 画面全体を覆うサイズ・カメラは自分の環境に合わせて
  sprite_->Initialize(camera, "resources/whiteTexture.png");
  sprite_->SetAnchor(0.0f, 0.0f);
  sprite_->SetPosition(0.0f, 0.0f);
  sprite_->SetSize(1280.0f, 720.0f);

  alpha_ = 0.0f;
  state_ = State::None;
}
void Fade::StartFadeIn(float duration) {
  state_ = State::FadeIn;
  duration_ = duration;
  timer_ = 0.0f;
  alpha_ = 1.0f; // 最初真っ黒 → 透明へ
}

void Fade::StartFadeOut(float duration) {
  state_ = State::FadeOut;
  duration_ = duration;
  timer_ = 0.0f;
  alpha_ = 0.0f; // 最初透明 → 真っ黒へ
}

void Fade::Update(float deltaTime) {
  if (state_ == State::None) {
    return;
  }

  timer_ += deltaTime;
  float t = duration_ > 0.0f ? timer_ / duration_ : 1.0f;
  if (t >= 1.0f) {
    t = 1.0f;
    if (state_ == State::FadeIn) {
      alpha_ = 0.0f;
    } else {
      alpha_ = 1.0f;
    }
    state_ = State::None;
  } else {
    if (state_ == State::FadeIn) {
      alpha_ = 1.0f - t;
    } else {
      alpha_ = t;
    }
  }
}

void Fade::Draw() {
  if (!sprite_ || alpha_ <= 0.0f) {
    return;
  }

  // 黒＋α で上からかぶせる
  sprite_->SetColor({0.0f, 0.0f, 0.0f, alpha_});
  sprite_->Draw();
}