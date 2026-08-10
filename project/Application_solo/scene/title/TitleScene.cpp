#include "TitleScene.h"

#include "Framework/SceneManager.h"
#include "Framework/SceneSerializer.h"
#include "Irufemi.h"

// デストラクタ
TitleScene::~TitleScene() {
}

// 初期化
void TitleScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // JSONからのロードは SceneManager が自動で行うため、ここでは手動で呼ばない
    
}

// 更新
void TitleScene::Update() {
    BaseScene::Update();

    if (IsKeyPressed(VK_SPACE) || IsButtonPressed(XINPUT_GAMEPAD_A)) {
        engine_->GetSceneManager()->TransitionTo("InGame", SceneTransition::Type::Fade, 1.0f);
    }
}

void TitleScene::Draw() {
    BaseScene::Draw();
}

void TitleScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}

