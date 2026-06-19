#include "SelectScene.h"
#include "Framework/SceneManager.h"
#include "Irufemi.h"

// デストラクタ
SelectScene::~SelectScene() = default;

// 初期化
void SelectScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);
}

// 更新
void SelectScene::Update() {
    BaseScene::Update();

    // BackSpaceキーで遷移テスト
    if (IsKeyPressed(VK_F8)) {
        engine_->GetSceneManager()->TransitionTo("Title", SceneTransition::Type::Slide, 1.0f);
    }
}

void SelectScene::Draw() {
    BaseScene::Draw();
}

void SelectScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}
