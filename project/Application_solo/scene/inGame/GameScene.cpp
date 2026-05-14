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

    // C++でのハードコードを廃止し、エディタで保存したJSONをロードする
    // ※ エディタで作成・保存した GameScene.json がロードされます。
    // ※ 初回起動時にファイルが無い場合は空のシーンから始まります。
    SceneSerializer::Load(this, "GameScene");
}

// 更新
void GameScene::Update() {
    BaseScene::Update(); // これにより GameObject 群の Update が呼ばれる

    // BackSpaceキーで遷移テスト
    if (IsKeyPressed(VK_BACK)) {
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
