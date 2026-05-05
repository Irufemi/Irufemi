#include "ClearScene.h"

#include "Framework/SceneManager.h"

#include "Irufemi.h"

#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"

ClearScene::~ClearScene() {

}

void ClearScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // シーン固有のカメラ位置に調整
    cameraManager_->GetActiveCamera()->SetTranslate({ 0.0f, 0.0f, -10.0f });
    cameraManager_->GetActiveCamera()->UpdateMatrix();

    // 「Clear!!」文字の初期化
    clearTextC_ = std::make_unique<ObjClass>();
    clearTextC_->Initialize(cameraManager_->GetActiveCamera(), "Clear/text_C.obj");
    clearTextL_ = std::make_unique<ObjClass>();
    clearTextL_->Initialize(cameraManager_->GetActiveCamera(), "Clear/text_l.obj");
    clearTextE_ = std::make_unique<ObjClass>();
    clearTextE_->Initialize(cameraManager_->GetActiveCamera(), "Clear/text_e.obj");
    clearTextA_ = std::make_unique<ObjClass>();
    clearTextA_->Initialize(cameraManager_->GetActiveCamera(), "Clear/text_a.obj");
    clearTextR_ = std::make_unique<ObjClass>();
    clearTextR_->Initialize(cameraManager_->GetActiveCamera(), "Clear/text_r.obj");
    clearTextEx_ = std::make_unique<ObjClass>();
    clearTextEx_->Initialize(cameraManager_->GetActiveCamera(), "Clear/text_!!.obj");

    clearTextC_->SetScale({ 1.5f, 1.5f, 1.5f });
    clearTextL_->SetScale({ 1.5f, 1.5f, 1.5f });
    clearTextE_->SetScale({ 1.5f, 1.5f, 1.5f });
    clearTextA_->SetScale({ 1.5f, 1.5f, 1.5f });
    clearTextR_->SetScale({ 1.5f, 1.5f, 1.5f });
    clearTextEx_->SetScale({ 1.5f, 1.5f, 1.5f });

    // 「Push to Space」文字の初期化
    textPushToSpace_ = std::make_unique<ObjClass>();
    textPushToSpace_->Initialize(cameraManager_->GetActiveCamera(), "text_pushtospace/text_pushtospace.obj");
    textPushToSpace_->SetPosition({ 0.0f, -2.5f, 0.0f });
    textPushToSpace_->SetScale({ 1.0f, 1.0f, 1.0f });
}

void ClearScene::Update() {

    BaseScene::Update();

    // =====
    // ↓ゲームの更新
    // =====

    // Clear文字のアニメーション（ポップなバウンド）
    const float clearBaseY = 1.0f;
    const float clearPositionsX[6] = { -3.5f, -2.1f, -0.7f, 0.7f, 2.1f, 3.5f };

    ObjClass* clearTexts[6] = {
        clearTextC_.get(), clearTextL_.get(), clearTextE_.get(),
        clearTextA_.get(), clearTextR_.get(), clearTextEx_.get()
    };

    for (int i = 0; i < 6; ++i) {
        if (!clearTexts[i]) continue;
        
        // 文字ごとに位相をずらす（少し早めのテンポ）
        float phase = animationTime_ * 5.0f - i * 0.5f;
        
        // 1. ポップに跳ねる（絶対値のサイン波でバウンド）
        float offsetY = std::abs(std::sin(phase)) * 0.8f;
        
        // 2. 左右に少しだけ揺らす
        float rotZ = std::cos(phase) * 0.15f;
        
        clearTexts[i]->SetPosition({ clearPositionsX[i], clearBaseY + offsetY, 0.0f });
        clearTexts[i]->SetRotate({ 0.0f, 0.0f, rotZ });
    }

    // 「Push to Space」文字の明滅
    if (textPushToSpace_) {
        animationTime_ += 1.0f / 60.0f;
        float alpha = 1.0f;
        isDrawPushToSpace_ = true;

        if (!isChangingScene_) {
            // 待機中：ゆっくりとした明滅
            alpha = 0.6f + std::sin(animationTime_ * 3.0f) * 0.4f;
        } else {
            // 決定後：高速フラッシュ
            isDrawPushToSpace_ = (std::sin(animationTime_ * 40.0f) > 0.0f);
        }
        textPushToSpace_->SetAlpha(alpha);
    }

    // Spaceキーが押されたらフラグを立てる
    if (!isChangingScene_ && IsKeyPressed(VK_SPACE)) {
        isChangingScene_ = true;
        transitionDelayTimer_ = 0.0f;
    }

    // 遷移フラグが立っていればタイマーを進める
    if (isChangingScene_ && !isTransitionRequested_) {
        transitionDelayTimer_ += 1.0f / 60.0f;
        if (transitionDelayTimer_ >= 0.8f) {
            engine_->GetSceneManager()->TransitionTo("Title", SceneTransition::Type::Fade, 1.0f);
            isTransitionRequested_ = true;
        }
    }

    // =====
    // ↑ゲームの更新
    // =====
}

void ClearScene::Draw() {
    // --- 3D描画 ---
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);

    if (clearTextC_) clearTextC_->Draw();
    if (clearTextL_) clearTextL_->Draw();
    if (clearTextE_) clearTextE_->Draw();
    if (clearTextA_) clearTextA_->Draw();
    if (clearTextR_) clearTextR_->Draw();
    if (clearTextEx_) clearTextEx_->Draw();

    if (textPushToSpace_ && isDrawPushToSpace_) {
        textPushToSpace_->Draw();
    }
}

void ClearScene::DrawDebugTab() {
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


