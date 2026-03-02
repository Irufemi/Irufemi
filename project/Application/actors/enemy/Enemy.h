#pragma once
#include <memory>
#include <array>
#include "Body.h"
#include "HeadLeft.h"
#include "HeadMid.h"
#include "HeadRight.h"

class Camera;

class Enemy {

public:
  ~Enemy();

  void Initialize(Camera *camera);

  void Update();
  void Draw();

private:
  std::array<std::unique_ptr<Body>, 3> bodies_;

  std::unique_ptr<HeadLeft> headLeft_ = nullptr;
  std::unique_ptr<HeadMid> headMid_ = nullptr;
  std::unique_ptr<HeadRight> headRight_ = nullptr;

  bool isActive_ = false;
};
