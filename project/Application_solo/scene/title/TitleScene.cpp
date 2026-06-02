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

    // 学校の資料に基づくポストエフェクト（ラジアルブラー）の適用
    engine_->SetPostProcessMode(IrufemiEngine::PostProcessMode::RadialBlur);
    auto& radialBlurParams = engine_->GetRadialBlurParams();
    radialBlurParams.center = { 0.5f, 0.5f }; // 放射状の基準となる中心点 (UV空間: 0.5で画面中央)
    radialBlurParams.blurWidth = 0.02f;       // ぼかしの強さ・幅
    radialBlurParams.numSamples = 10;         // サンプリング回数 (多いほど滑らかだが重くなる)

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

