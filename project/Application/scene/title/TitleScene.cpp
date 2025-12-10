#include "TitleScene.h"

#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"
#include "scene/SceneManager.h"

// 初期化
void TitleScene::Initialize(IrufemiEngine *engine) {

  engine_ = engine;

  camera_ = std::make_unique<Camera>();
  camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
  camera_->SetTranslate(Vector3{0.0f, 0.0f, -10.0f});

  // 重要：SetTranslate の後で行列を確実に更新しておく
  camera_->UpdateMatrix();

  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize(engine_->GetInputManager(),
                           engine_->GetClientWidth(),
                           engine_->GetClientHeight());
  debugMode = false;

  pointLight_ = std::make_unique<PointLightClass>();
  pointLight_->Initialize();
  pointLight_->SetPos(Vector3{0.0f, -5.0f, 0.0f});
  engine_->GetDrawManager()->SetPointLightClass(pointLight_.get());

  spotLight_ = std::make_unique<SpotLightClass>();
  spotLight_->Initialize();
  spotLight_->SetIntensity(0.0f);
  engine_->GetDrawManager()->SetSpotLightClass(spotLight_.get());

  fade_.Initialize(engine_, camera_.get());
  fade_.StartFadeIn(0.5f);

  //SEの初期化
  cursolSE_.Initialize("resources/se/cursol.mp3");
  decisionSE_.Initialize("resources/se/decision.mp3");

  //タイトルBGMの初期化
  titleBGM_.Initialize("resources/bgm/titleBGM.mp3");
  titleBGM_.PlayFixed();

  deciding_ = false;
  decideTimer_ = 0.0f;
}

// 更新
void TitleScene::Update() {
  // カメラの通常更新
  if (debugMode) {
    debugCamera_->Update();
    camera_->SetViewMatrix(debugCamera_->GetCamera().GetViewMatrix());
    camera_->SetPerspectiveFovMatrix(
        debugCamera_->GetCamera().GetPerspectiveFovMatrix());
  } else {
    camera_->Update("Camera", {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
  }

  if (!fade_.IsFading()) {
    if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE) ||
        engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {

      // 次に行くシーン名をセットして、フェードアウト開始
      nextSceneName_ = "InGame";
      fade_.StartFadeOut(0.5f);
    }
  }

  fade_.Update(1.0f / 60.0f);

  if (!fade_.IsFading() && !nextSceneName_.empty()) {
    engine_->GetSceneManager()->Request(nextSceneName_.c_str());
    nextSceneName_.clear();
  }
}

void TitleScene::Draw() {

  // 3D
  engine_->SetBlend(BlendMode::kBlendModeNormal);
  engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
  engine_->ApplyPSO();

  // 2D

  engine_->SetBlend(BlendMode::kBlendModeNormal);
  engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
  engine_->ApplySpritePSO();

  fade_.Draw();
}