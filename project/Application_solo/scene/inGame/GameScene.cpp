#include "GameScene.h"
#include "Framework/SceneManager.h"
#include "Irufemi.h"

// デストラクタ
GameScene::~GameScene() = default;

// 初期化
void GameScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);
}

// 更新
void GameScene::Update() {
    BaseScene::Update();

    // BackSpaceキーで遷移テスト
    if (IsKeyPressed(VK_BACK)) {
        engine_->GetSceneManager()->TransitionTo("Pause", SceneTransition::Type::Slide, 1.0f);
    }
}

void GameScene::Draw() {
}

void GameScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}
