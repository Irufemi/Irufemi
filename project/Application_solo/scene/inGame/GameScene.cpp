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
    testObject_ = std::make_unique<GameObject>("TestPlane");

    // TransformComponent を追加
    auto* transform = testObject_->AddComponent<TransformComponent>();
    transform->position_ = { 0.0f, 0.0f, 0.0f }; // 原点に配置

    // MeshRendererComponent を追加してモデルを読み込む
    auto* renderer = testObject_->AddComponent<MeshRendererComponent>();
    renderer->LoadModel("plane.obj");

    // 全コンポーネントの初期化
    testObject_->Initialize();
}

// 更新
void GameScene::Update() {
    BaseScene::Update();

    // ECSの更新
    if (testObject_) {
        testObject_->Update();
    }

    // BackSpaceキーで遷移テスト
    if (IsKeyPressed(VK_BACK)) {
        engine_->GetSceneManager()->TransitionTo("Pause", SceneTransition::Type::Slide, 1.0f);
    }
}

void GameScene::Draw() {
    // ECSの描画パケット登録
    if (testObject_) {
        testObject_->Draw();
    }
}

void GameScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();

    // EditorMode の時だけインスペクターを表示する
#if defined EditorMode
    if (testObject_) {
        auto* transform = testObject_->GetComponent<TransformComponent>();
        auto* renderer = testObject_->GetComponent<MeshRendererComponent>();

        if (transform) transform->OnInspectorGUI();
        if (renderer) renderer->OnInspectorGUI();
    }
#endif
#endif
}
