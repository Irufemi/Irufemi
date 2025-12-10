#pragma once

#include "scene/IScene.h"

#include "2D/Sprite.h"
#include "3D/CylinderClass.h"
#include "3D/ObjClass.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "audio/Bgm.h"
#include "audio/Se.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "effect/Fade.h"
#include "math/shape/LinePrimitive.h"
#include <memory>
#include <vector>

/// <summary>
/// タイトル
/// </summary>
class TitleScene : public IScene {

public: // メンバ関数
  void Initialize(IrufemiEngine *engine) override;
  void Update() override;
  void Draw() override;

private: // メンバ変数
  IrufemiEngine *engine_ = nullptr;

  std::unique_ptr<Camera> camera_ = nullptr;

  std::unique_ptr<DebugCamera> debugCamera_ = nullptr;
  bool debugMode = false;

  // タイトル文字
  std::unique_ptr<Sprite> titleText_ = nullptr;

  // プッシュ文字
  std::unique_ptr<Sprite> pushText_ = nullptr;

  // アイドリングアニメーション用タイマー
  float idleAnimTimer_ = 0.0f;

  // 遷移フェード
  Fade fade_;
  std::string nextSceneName_;
    std::unique_ptr<PointLightClass> pointLight_ = nullptr;
    std::unique_ptr<SpotLightClass> spotLight_ = nullptr;

    //SEの初期化
    Se cursolSE_;
    Se decisionSE_;

    //タイトルBGMの初期化
	Bgm titleBGM_;

    bool deciding_ = false;
    float decideTimer_ = 0.0f;

};