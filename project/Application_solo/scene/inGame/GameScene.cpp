#include "GameScene.h"
#include "Framework/SceneManager.h"
#include "Irufemi.h"

// ECSコンポーネントのインクルード
#include "Framework/GameObject.h"
#include "Framework/TransformComponent.h"
#include "Framework/MeshRendererComponent.h"

// デストラクタ
GameScene::~GameScene() = default;

// 初期化
void GameScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // テスト用の GameObject を生成
    auto testObject = std::make_shared<GameObject>("TestPlane");

    // TransformComponent を追加
    auto* transform = testObject->AddComponent<TransformComponent>();
    transform->position_ = { 0.0f, 0.0f, 0.0f }; // 原点に配置

    // MeshRendererComponent を追加してモデルを読み込む
    auto* renderer = testObject->AddComponent<MeshRendererComponent>();
    renderer->LoadModel("plane.obj");

    // 全コンポーネントの初期化
    testObject->Initialize();

    // シーンのリストに登録
    AddGameObject(testObject);
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
