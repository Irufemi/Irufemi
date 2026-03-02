#pragma once
#include "3D/ObjClass.h"
#include <memory>

class Camera;

class Enemy {

public:
  ~Enemy();

  void Initialize(Camera *camera);

  void Update();
  void Draw();

private:
  std::unique_ptr<ObjClass> body_ = nullptr;
  bool isActivebody_ = false;

  std::unique_ptr<ObjClass> head_ = nullptr;
  bool isActivehead_ = false;
};
