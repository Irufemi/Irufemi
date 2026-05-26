#include "TitleScene.h"

#include "Framework/SceneManager.h"
#include "Framework/SceneSerializer.h"
#include "Irufemi.h"

#include "Engine/Graphics/PostProcess/PostProcessManager.h"

// デストラクタ
TitleScene::~TitleScene() {
    if (engine_ && engine_->GetPostProcessManager()) {
        engine_->GetPostProcessManager()->RemoveActiveMode(PostProcessMode::Vignette);
    }
}

// 初期化
void TitleScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // JSONからのロードは SceneManager が自動で行うため、ここでは手動で呼ばない
    
    // ポストプロセスの有効化（Vignetteの適用）
    if (auto* pp = engine_->GetPostProcessManager()) {
        pp->AddActiveMode(PostProcessMode::Vignette);
        pp->GetVignetteParams().scale = 16.0f; // 初期値
        pp->GetVignetteParams().power = 0.8f;  // 初期値
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
            // Vignette
            float& scale = pp->GetVignetteParams().scale;
            float& power = pp->GetVignetteParams().power;
            ImGui::SliderFloat("Vignette Scale", &scale, 1.0f, 32.0f);
            ImGui::SliderFloat("Vignette Power", &power, 0.1f, 5.0f);
        }
        ImGui::EndTabItem();
    }
#endif
}

