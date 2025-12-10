#pragma once
#include "2D/Sprite.h"
#include <memory>

class Camera;

class IrufemiEngine;

class Fade {
  enum class State {
    None,
    FadeIn,
    FadeOut,
  };

public:
  Fade() = default;

  void Initialize(IrufemiEngine *engine,Camera*camera);

  void StartFadeIn(float duration);
  void StartFadeOut(float duration);

  void Update(float deltaTime);

  void Draw();

  bool IsFading() const { return state_ != State::None; }

private:
  std::unique_ptr<Sprite> sprite_;

  State state_ = State::None;
  float timer_ = 0.0f;
  float duration_ = 1.0f;
  float alpha_ = 0.0f;

  IrufemiEngine *engine_ = nullptr;
};
