#include "TitleScene.h"

#include "Framework/SceneManager.h"
#include "Framework/SceneSerializer.h"
#include "Irufemi.h"

// デストラクタ
TitleScene::~TitleScene() {
    if (engine_) {
        engine_->SetPostProcessMode(IrufemiEngine::PostProcessMode::None);
    }
}

// 初期化
void TitleScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // 学校の資料に基づくポストエフェクト（DepthBasedOutline / Prewitt Filter）の適用
    engine_->SetPostProcessMode(IrufemiEngine::PostProcessMode::DepthBasedOutline);
    // ※DepthBasedOutlineに必要なパラメータ（逆投影行列など）はエンジン内部で自動設定されます

    // JSONからのロードは SceneManager が自動で行うため、ここでは手動で呼ばない
    
}

// 更新
void TitleScene::Update() {
    BaseScene::Update();

    // BackSpaceキーで遷移テスト
    if (IsKeyPressed(VK_F8)) {
        engine_->GetSceneManager()->TransitionTo("InGame", SceneTransition::Type::Slide, 1.0f);
    }
}

void TitleScene::Draw() {
    BaseScene::Draw();
}

void TitleScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}

