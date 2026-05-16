#include "PauseScene.h"
#include "Framework/SceneManager.h"
#include "Irufemi.h"

// デストラクタ
PauseScene::~PauseScene() = default;

// 初期化
void PauseScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);
}

// 更新
void PauseScene::Update() {
    BaseScene::Update();

    // BackSpaceキーで遷移テスト
    if (IsKeyPressed(VK_F8)) {
        engine_->GetSceneManager()->TransitionTo("Clear", SceneTransition::Type::Slide, 1.0f);
    }
}

void PauseScene::Draw() {
    BaseScene::Draw();
}

void PauseScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}
