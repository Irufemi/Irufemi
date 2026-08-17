#include "Scenes/stageSelect/SelectScene.h"
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

}

void SelectScene::Draw() {
    BaseScene::Draw();
}

void SelectScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}
