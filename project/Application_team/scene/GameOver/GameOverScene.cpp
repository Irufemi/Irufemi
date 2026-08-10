#include "GameOverScene.h"
#include "Framework/SceneManager.h"
#include "Irufemi.h"

// デストラクタ
GameOverScene::~GameOverScene() = default;

// 初期化
void GameOverScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);
}

// 更新
void GameOverScene::Update() {
    BaseScene::Update();

    // BackSpaceキーで遷移テスト
    if (IsKeyPressed(VK_F8)) {
        engine_->GetSceneManager()->TransitionTo("Title", SceneTransition::Type::Slide, 1.0f);
    }
}

void GameOverScene::Draw() {
    BaseScene::Draw();
}

void GameOverScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}
