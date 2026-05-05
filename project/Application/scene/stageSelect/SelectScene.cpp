#include "SelectScene.h"
#include "Framework/SceneManager.h"
#include "Irufemi.h"

SelectScene::~SelectScene() {

}

void SelectScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // SelectScene固有の初期化があればここに記述
}

void SelectScene::Update() {
    BaseScene::Update(); // カメラやフレームデータ更新

    // =====
    // ↓ゲームの更新
    // =====



    // =====
    // ↑ゲームの更新
    // =====

    // エンターキー/Aボタンが押されていたらゲームへ
    if (IsKeyPressed(VK_RETURN) || IsButtonPressed(XINPUT_GAMEPAD_A)) {
        engine_->GetSceneManager()->TransitionTo("InGame", SceneTransition::Type::Fade, 1.0f);
    }
}

void SelectScene::Draw() {
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
}

void SelectScene::DrawDebugTab() {
#if defined USE_IMGUI
    BaseScene::DrawDebugTab();

    // Texture タブ
    if (ImGui::BeginTabItem("Texture")) {
        if (ImGui::Button("allLoadActivate")) {
            engine_->GetTextureManager()->LoadAllFromFolder("resources/");
        }
        ImGui::EndTabItem();
    }
#endif
}


