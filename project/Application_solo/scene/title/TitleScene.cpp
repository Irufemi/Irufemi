#include "TitleScene.h"

#include "Framework/SceneManager.h"
#include "Irufemi.h"

// デストラクタ
TitleScene::~TitleScene() = default;

// 初期化
void TitleScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);
}

// 更新
void TitleScene::Update() {
    BaseScene::Update();

    // BackSpaceキーで遷移テスト
    if (IsKeyPressed(VK_BACK)) {
        engine_->GetSceneManager()->TransitionTo("InGame", SceneTransition::Type::Slide, 1.0f);
    }
}

void TitleScene::Draw() {
}

void TitleScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}
