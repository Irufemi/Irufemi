#include "Scenes/inGame/GameScene.h"
#include "Framework/SceneManager.h"
#include "Irufemi.h"
#include "Engine/Graphics/PostProcess/PostProcessManager.h"

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
    
    // --- ポストプロセスの適用（画面全体を派手にする演出） ---
    auto pp = engine_->GetPostProcessManager();
    
    if (!pp->HasActiveMode(PostProcessMode::Bloom)) {
        pp->AddActiveMode(PostProcessMode::Bloom);
    }
    if (!pp->HasActiveMode(PostProcessMode::Vignette)) {
        pp->AddActiveMode(PostProcessMode::Vignette);
    }

    // Bloom（発光）の設定
    auto& bloom = pp->GetBloomParams();
    bloom.threshold = 0.6f;  // 発光のしきい値
    bloom.intensity = 1.8f;  // ブルーム強度（派手にするため強め）
    bloom.sigma = 4.0f;      // ぼかしを広くする

    // Vignette（画面端の黒ずみ）の設定
    auto& vignette = pp->GetVignetteParams();
    vignette.radius = 0.8f;   // 少し広めから減衰開始
    vignette.softness = 0.5f; // 滑らかに
    vignette.color = { 0.0f, 0.0f, 0.0f, 1.0f };
}

// 更新
void GameScene::Update() {
    BaseScene::Update(); // これにより GameObject 群の Update が呼ばれる


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
