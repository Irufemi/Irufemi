#include "TitleScene.h"

#include "Framework/SceneManager.h"
#include "Framework/SceneSerializer.h"
#include "Irufemi.h"

#include "Engine/Graphics/PostProcess/PostProcessManager.h"

// デストラクタ
TitleScene::~TitleScene() {
    if (engine_ && engine_->GetPostProcessManager()) {
        engine_->GetPostProcessManager()->RemoveActiveMode(PostProcessMode::Smoothing);
    }
}

// 初期化
void TitleScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // JSONからのロードは SceneManager が自動で行うため、ここでは手動で呼ばない
    
    // ポストプロセスの有効化（Smoothingの適用）
    if (auto* pp = engine_->GetPostProcessManager()) {
        pp->AddActiveMode(PostProcessMode::Smoothing);
        pp->GetSmoothingParams().kernelSize = 5; // 初期値として5x5 BoxFilterを設定
    }
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

    // PostEffect タブ
    if (ImGui::BeginTabItem("PostEffect")) {
        if (auto* pp = engine_->GetPostProcessManager()) {
            int& kernel = pp->GetSmoothingParams().kernelSize;
            ImGui::SliderInt("BoxFilter Kernel", &kernel, 1, 15);
            if (kernel % 2 == 0) {
                kernel += 1; // 常に奇数になるように補正
            }
        }
        ImGui::EndTabItem();
    }
#endif
}

