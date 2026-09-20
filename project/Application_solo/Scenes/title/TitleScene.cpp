#include "Scenes/title/TitleScene.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/SceneSerializer.h"
#include "Irufemi.h"

#include "Platform/Input/InputManager.h"

// デストラクタ
TitleScene::~TitleScene() {}

// 初期化
void TitleScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // JSONからのロードは SceneManager が自動で行うため、ここでは手動で呼ばない
}

// 更新
void TitleScene::Update() {
    BaseScene::Update();

    auto inputManager = engine_ ? engine_->GetInputManager() : nullptr;
    if (inputManager) {
        if (inputManager->IsKeyPressed(VK_SPACE) || inputManager->IsButtonPressed(XINPUT_GAMEPAD_A)) {
            engine_->GetSceneManager()->TransitionTo("InGame", SceneTransition::Type::Fade, 1.0f);
        }

        // オプション画面のテスト用呼び出し
        if (inputManager->IsKeyPressed('O') || inputManager->IsButtonPressed(XINPUT_GAMEPAD_START)) {
            engine_->GetSceneManager()->PushScene("OptionsScene");
        }
    }
}

void TitleScene::Draw() {
    BaseScene::Draw();
}
