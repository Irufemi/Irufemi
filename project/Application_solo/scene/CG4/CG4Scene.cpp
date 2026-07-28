#include "CG4Scene.h"
#include "Framework/SceneManager.h"
#include "Irufemi.h"

// ECSコンポーネントのインクルード
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"

// デストラクタ
CG4Scene::~CG4Scene() = default;

// 初期化
void CG4Scene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // シーン初期化時に必要なオブジェクト等があればここに追加する
}

// 更新
void CG4Scene::Update() {
    BaseScene::Update();

    // BackSpaceキーで遷移テスト (例)
    if (IsKeyPressed(VK_BACK)) {
        engine_->GetSceneManager()->TransitionTo("Title", SceneTransition::Type::Slide, 1.0f);
    }
}

void CG4Scene::Draw() {
    BaseScene::Draw();
}

void CG4Scene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}
