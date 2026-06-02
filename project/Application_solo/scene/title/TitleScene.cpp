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

    // 学校の資料に基づくポストエフェクト（ディゾルブ）の適用
    engine_->SetPostProcessMode(IrufemiEngine::PostProcessMode::Dissolve);
    auto& dissolveParams = engine_->GetDissolveParams();
    dissolveParams.threshold = 0.5f;                              // 消失しきい値 (0.0:完全に表示 〜 1.0:完全に消失)
    dissolveParams.edgeColor = { 0.0f, 1.0f, 1.0f, 1.0f };        // 境界線の発光色 (RGBA)
    dissolveParams.edgeRange = 0.05f;                             // 境界線の幅
    dissolveParams.noiseType = 0;                                 // 使用するノイズテクスチャの種類

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

