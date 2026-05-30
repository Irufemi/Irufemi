#include "GameOverScene.h"

#include "Framework/SceneManager.h"

#include "Irufemi.h"

#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"
#include "Engine/Graphics/PostProcess/PostProcessManager.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"

GameOverScene::~GameOverScene() {
    // シーン破棄時に自身のポストプロセスのみを取り除く
    if (engine_ && engine_->GetPostProcessManager()) {
        auto* pp = engine_->GetPostProcessManager();
        pp->RemoveActiveMode(PostProcessMode::Grayscale);
        pp->RemoveActiveMode(PostProcessMode::Vignette);
        pp->RemoveActiveMode(PostProcessMode::Bloom);
    }

    // 画面のクリアカラーをデフォルト（Cornflower blue）に戻す
    if (engine_) {
        engine_->SetClearColor(0.1f, 0.25f, 0.5f, 1.0f);
    }
}

void GameOverScene::OnEnter() {
    if (bgm_) bgm_->PlayFixed();
}

void GameOverScene::OnSuspend() {
    if (bgm_) bgm_->Pause();
}

void GameOverScene::OnResume() {
    if (bgm_) bgm_->Resume();
}

void GameOverScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    bgm_ = std::make_unique<Bgm>();
    bgm_->Initialize("resources/BGM/GameOver.mp3", "GameOverBGM", true);

    // シーン固有のカメラ位置に調整
    engine_->GetCameraManager()->GetActiveCamera()->SetTranslate({ 0.0f, 0.0f, -10.0f });
    engine_->GetCameraManager()->GetActiveCamera()->UpdateMatrix();

    // 画面のクリアカラーを真っ暗な深紅に変更（絶望感・血のイメージ）
    engine_->SetClearColor(0.05f, 0.0f, 0.0f, 1.0f);

    // 「GameOver...」文字の初期化
    goTextG_ = std::make_unique<ObjClass>();
    goTextG_->Initialize("gameOver/text_G.obj");
    goTextA_ = std::make_unique<ObjClass>();
    goTextA_->Initialize("gameOver/text_a.obj"); 
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

    // 選択肢（やりなおし、タイトルへ戻る）の初期化
    objRetry_ = std::make_unique<ObjClass>();
    objRetry_->Initialize("text_retry/text_retry.obj");
    objRetry_->SetPosition({ -3.0f, -2.5f, 0.0f });
    objRetry_->SetScale({ 1.0f, 1.0f, 1.0f });

    objBackToTitle_ = std::make_unique<ObjClass>();
    objBackToTitle_->Initialize("text_backToTitle/text_backToTitle.obj");
    objBackToTitle_->SetPosition({ 3.0f, -2.5f, 0.0f });
    objBackToTitle_->SetScale({ 1.0f, 1.0f, 1.0f });

    // UISelectionGroupの設定
    gameOverSelection_.SetHorizontalMode(true);
    gameOverSelection_.AddItem(objRetry_.get());
    gameOverSelection_.AddItem(objBackToTitle_.get());
    gameOverSelection_.SetActiveBaseColor({ 1.0f, 0.1f, 0.1f, 1.0f }); // 発光する赤
    gameOverSelection_.SetInactiveColor({ 0.3f, 0.0f, 0.0f, 0.8f });

    // ポストプロセスの有効化（絶望感の演出）
    if (auto* pp = engine_->GetPostProcessManager()) {
        // ※Grayscaleは火の粉（Embers）の赤みを消してしまうため削除
        pp->AddActiveMode(PostProcessMode::Vignette);
        pp->AddActiveMode(PostProcessMode::Bloom);
        pp->GetVignetteParams().scale = 12.0f;
        pp->GetVignetteParams().power = 1.0f;
        pp->GetBloomParams().threshold = 0.5f;
        pp->GetBloomParams().intensity = 2.0f;
    }

    // 灰パーティクルの初期化
    embersParticles_ = std::make_unique<GPUParticleSystem>();
    embersParticles_->Initialize("resources/circle.png");
    embersParticles_->SetBlend(BlendMode::kBlendModeAdd);
    embersParticles_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    embersParticles_->SetCull(PSOManager::CullMode::None);
    embersParticles_->SetParticleLife(3.0f, 6.0f);
    embersParticles_->SetParticleScale({0.1f, 0.1f, 0.1f}, {0.3f, 0.3f, 0.3f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
    embersParticles_->SetColor({1.0f, 0.2f, 0.1f, 0.8f});
    embersParticles_->SetStartColor({1.0f, 0.4f, 0.1f, 1.0f}, {1.0f, 0.1f, 0.0f, 1.0f});
    embersParticles_->SetEndColor({0.2f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f});
    embersParticles_->SetGravity(-0.02f); // 上にゆっくり昇る (HLSLではマイナスが上への加速)
    embersParticles_->SetDamping(0.01f);
    
    // デフォルトで奥（Z方向）へ爆速で飛んでいくのを防ぎ、ふわふわ舞い上がるようにする
    embersParticles_->SetDirection({0.0f, 1.0f, 0.0f});
    embersParticles_->SetVelocity(0.02f); 
    embersParticles_->SetSpread(1.0f);
    
    embersParticles_->SetEmit(true);
}

void GameOverScene::Update() {

    BaseScene::Update();

    // =====
    // ↓ゲームの更新
    // =====

    // GameOver文字のアニメーション（重苦しいゆっくりとした浮遊）
    goTextAnimator_.Update(1.0f / 60.0f);
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

    // カメラのゆっくりとした引き（次第に遅くなり、-15.0 付近で止まる）
    cameraZ_ += (-15.0f - cameraZ_) * 0.2f * (1.0f / 60.0f);
    engine_->GetCameraManager()->GetActiveCamera()->SetTranslate({ 0.0f, 0.0f, cameraZ_ });
    engine_->GetCameraManager()->GetActiveCamera()->UpdateMatrix();

    // フェードインのタイマー
    introTimer_ += 1.0f / 60.0f;
    float alpha = std::clamp(introTimer_ * 0.3f, 0.0f, 1.0f); // 約3.3秒かけてフェードイン
    Vector4 textColor = { 1.0f, 0.1f, 0.1f, alpha }; // 発光する赤

    for (int i = 0; i < 9; ++i) {
        if (!goTexts[i]) continue;
        
        // 重々しいゆっくりとした動き
        float phaseOffset = i * 0.3f;
        float offsetY = goTextAnimator_.GetFloatOffset(0.3f, 1.5f, phaseOffset);
        
        goTexts[i]->SetPosition({ goPositionsX[i], goBaseY + offsetY, 0.0f });
        goTexts[i]->SetRotate({ 0.0f, 0.0f, 0.0f });
        goTexts[i]->SetScale({ 1.2f, 1.2f, 1.2f });
        goTexts[i]->SetColor(textColor);
    }

    // 灰パーティクルの更新
    auto cam = engine_->GetCameraManager()->GetActiveCamera();
    Vector3 camPos = cam->GetTranslate();
    embersParticles_->SetBoxEmitter(camPos + Vector3{0.0f, -5.0f, 10.0f}, {15.0f, 2.0f, 5.0f}, 1, 0.05f);
    embersParticles_->Update();

    // 選択肢コントローラーの表示・更新（文字が出揃ってから）
    if (introTimer_ >= 3.5f) {
        // 1秒かけてフェードイン
        float uiAlpha = std::clamp((introTimer_ - 3.5f) * 1.0f, 0.0f, 1.0f);
        gameOverSelection_.SetActiveBaseColor({ 1.0f, 0.1f, 0.1f, 1.0f * uiAlpha });
        gameOverSelection_.SetInactiveColor({ 0.3f, 0.0f, 0.0f, 0.8f * uiAlpha });

        gameOverSelection_.Update(engine_->GetInputManager());

        if (gameOverSelection_.ShouldTransition()) {
            if (gameOverSelection_.GetSelectedIndex() == 0) {
                engine_->GetSceneManager()->TransitionTo("InGame", SceneTransition::Type::Fade, 1.0f);
            } else {
                engine_->GetSceneManager()->TransitionTo("Title", SceneTransition::Type::Fade, 1.0f);
            }
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

    if (embersParticles_) embersParticles_->Draw();

    if (introTimer_ >= 3.5f) {
        gameOverSelection_.Draw();
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


