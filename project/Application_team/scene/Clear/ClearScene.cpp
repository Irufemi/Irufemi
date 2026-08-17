#include "ClearScene.h"
#include "Framework/Scene/SceneManager.h"
#include "Irufemi.h"

// デストラクタ
ClearScene::~ClearScene() = default;

// 初期化
void ClearScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);
}

// 更新
void ClearScene::Update() {
    BaseScene::Update();

    // BackSpaceキーで遷移テスト
    if (IsKeyPressed(VK_F8)) {
        engine_->GetSceneManager()->TransitionTo("Select", SceneTransition::Type::Slide, 1.0f);
    }
}

void ClearScene::Draw() {
    BaseScene::Draw();
}

void ClearScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}
