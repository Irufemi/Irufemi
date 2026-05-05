#include "GameOverScene.h"

#include "Framework/SceneManager.h"

#include "Irufemi.h"

#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"

GameOverScene::~GameOverScene() {

}

void GameOverScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // シーン固有のカメラ位置に調整
    engine_->GetCameraManager()->GetActiveCamera()->SetTranslate({ 0.0f, 0.0f, -10.0f });
    engine_->GetCameraManager()->GetActiveCamera()->UpdateMatrix();

    // 「GameOver...」文字の初期化
    goTextG_ = std::make_unique<ObjClass>();
    goTextG_->Initialize("gameOver/text_G.obj");
    goTextA_ = std::make_unique<ObjClass>();
    goTextA_->Initialize("gameOver/text_q.obj"); // ※ファイル名text_q.objはaとして使用
    goTextM_ = std::make_unique<ObjClass>();
    goTextM_->Initialize("gameOver/text_m.obj");
    goTextE1_ = std::make_unique<ObjClass>();
    goTextE1_->Initialize("gameOver/text_e.obj");
    goTextO_ = std::make_unique<ObjClass>();
    goTextO_->Initialize("gameOver/text_O.obj");
    goTextV_ = std::make_unique<ObjClass>();
    goTextV_->Initialize("gameOver/text_v.obj");
    goTextE2_ = std::make_unique<ObjClass>();
    goTextE2_->Initialize("gameOver/text_e.obj");
    goTextR_ = std::make_unique<ObjClass>();
    goTextR_->Initialize("gameOver/text_r.obj");
    
    // text_dot.obj（...）の読み込み
    goTextDot_ = std::make_unique<ObjClass>();
    goTextDot_->Initialize("gameOver/text_dot.obj");

    // 「Push to Space」文字の初期化
    textPushToSpace_ = std::make_unique<ObjClass>();
    textPushToSpace_->Initialize("text_pushtospace/text_pushtospace.obj");
    textPushToSpace_->SetPosition({ 0.0f, -2.5f, 0.0f });
    textPushToSpace_->SetScale({ 1.0f, 1.0f, 1.0f });
}

void GameOverScene::Update() {

    BaseScene::Update();

    // =====
    // ↓ゲームの更新
    // =====

    // GameOver文字のアニメーション（重苦しいゆっくりとした浮遊）
    const float goBaseY = 1.0f;
    const float goPositionsX[9] = {
        -5.0f, -3.8f, -2.4f, -1.2f, // Game
         0.4f,  1.6f,  2.8f,  4.0f, // Over
         5.5f                       // ...
    };

    ObjClass* goTexts[9] = {
        goTextG_.get(), goTextA_.get(), goTextM_.get(), goTextE1_.get(),
        goTextO_.get(), goTextV_.get(), goTextE2_.get(), goTextR_.get(),
        goTextDot_.get()
    };

    for (int i = 0; i < 9; ++i) {
        if (!goTexts[i]) continue;
        
        // 重々しいゆっくりとした動き
        float phase = animationTime_ * 1.5f + i * 0.3f;
        float offsetY = std::sin(phase) * 0.3f;
        
        goTexts[i]->SetPosition({ goPositionsX[i], goBaseY + offsetY, 0.0f });
        goTexts[i]->SetRotate({ 0.0f, 0.0f, 0.0f });
        goTexts[i]->SetScale({ 1.2f, 1.2f, 1.2f });
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

void GameOverScene::Draw() {

    // --- 3D描画 ---
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);

    if (goTextG_) goTextG_->Draw();
    if (goTextA_) goTextA_->Draw();
    if (goTextM_) goTextM_->Draw();
    if (goTextE1_) goTextE1_->Draw();
    if (goTextO_) goTextO_->Draw();
    if (goTextV_) goTextV_->Draw();
    if (goTextE2_) goTextE2_->Draw();
    if (goTextR_) goTextR_->Draw();
    if (goTextDot_) goTextDot_->Draw();

    if (textPushToSpace_ && isDrawPushToSpace_) {
        textPushToSpace_->Draw();
    }
}

void GameOverScene::DrawDebugTab() {
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


