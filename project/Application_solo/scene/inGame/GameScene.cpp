#include "GameScene.h"
#include "Framework/SceneManager.h"
#include "Irufemi.h"

// ECSコンポーネントのインクルード
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/SceneSerializer.h"

// デストラクタ
GameScene::~GameScene() = default;

// 初期化
void GameScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // JSONからのロードは SceneManager が自動で行うため、ここでは手動で呼ばない

    // 課題用にグレースケールを適用
    engine_->GetPostProcessManager()->ClearActiveModes();
    engine_->GetPostProcessManager()->AddActiveMode(PostProcessMode::Grayscale);
}

// 更新
void GameScene::Update() {
    BaseScene::Update(); // これにより GameObject 群の Update が呼ばれる

    // BackSpaceキーで遷移テスト
    if (IsKeyPressed(VK_F8)) {
        engine_->GetSceneManager()->TransitionTo("Pause", SceneTransition::Type::Slide, 1.0f);
    }
}

void GameScene::Draw() {
    BaseScene::Draw(); // これにより GameObject 群の Draw が呼ばれる
}

void GameScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();

    // InspectorはEditorManager側に移管するため、ここでの描画は削除
#endif
}
