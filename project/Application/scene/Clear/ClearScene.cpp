#include "ClearScene.h"

#include "Framework/SceneManager.h"

#include "Irufemi.h"

#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"
#include "Resource/Audio/AudioManager.h"

ClearScene::~ClearScene() {
    // シーン破棄時に自身のポストプロセスのみを取り除く
    if (engine_ && engine_->GetPostProcessManager()) {
        auto* pp = engine_->GetPostProcessManager();
        pp->RemoveActiveMode(PostProcessMode::Bloom);
        pp->RemoveActiveMode(PostProcessMode::HSV);
        
        // 他のシーンに影響が出ないよう、変更したパラメータをデフォルトに戻す
        pp->GetHSVParams().saturation = 0.0f;
        pp->GetHSVParams().value = 0.0f;
    }
}

void ClearScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // ポストプロセスの有効化（華やか・祝祭感の演出）
    if (auto* pp = engine_->GetPostProcessManager()) {
        pp->AddActiveMode(PostProcessMode::Bloom);
        pp->AddActiveMode(PostProcessMode::HSV);
        
        // HSVで彩度を少し上げて、ポップで鮮やかな色彩にする
        pp->GetHSVParams().saturation = 0.2f;
        pp->GetHSVParams().value = 0.1f; // 少し明るくする
    }

    // シーン固有のカメラ位置に調整
    engine_->GetCameraManager()->GetActiveCamera()->SetTranslate({ 0.0f, 0.0f, -10.0f });
    engine_->GetCameraManager()->GetActiveCamera()->UpdateMatrix();

    // 「Clear!!」文字の初期化
    clearTextC_ = std::make_unique<ObjClass>();
    clearTextC_->Initialize("Clear/text_C.obj");
    clearTextL_ = std::make_unique<ObjClass>();
    clearTextL_->Initialize("Clear/text_l.obj");
    clearTextE_ = std::make_unique<ObjClass>();
    clearTextE_->Initialize("Clear/text_e.obj");
    clearTextA_ = std::make_unique<ObjClass>();
    clearTextA_->Initialize("Clear/text_a.obj");
    clearTextR_ = std::make_unique<ObjClass>();
    clearTextR_->Initialize("Clear/text_r.obj");
    clearTextEx_ = std::make_unique<ObjClass>();
    clearTextEx_->Initialize("Clear/text_!!.obj");

    clearTextC_->SetScale({ 1.5f, 1.5f, 1.5f });
    clearTextL_->SetScale({ 1.5f, 1.5f, 1.5f });
    clearTextE_->SetScale({ 1.5f, 1.5f, 1.5f });
    clearTextA_->SetScale({ 1.5f, 1.5f, 1.5f });
    clearTextR_->SetScale({ 1.5f, 1.5f, 1.5f });
    clearTextEx_->SetScale({ 1.5f, 1.5f, 1.5f });

    // 「Push to Space」文字の初期化
    textPushToSpace_ = std::make_unique<ObjClass>();
    textPushToSpace_->Initialize("text_pushtospace/text_pushtospace.obj");
    textPushToSpace_->SetPosition({ 0.0f, -2.5f, 0.0f });
    textPushToSpace_->SetScale({ 1.0f, 1.0f, 1.0f });

    // プロンプトコントローラーに登録
    promptController_.SetTarget(textPushToSpace_.get());

    // 祝祭パーティクルの初期化
    confettiParticles_ = std::make_unique<GPUParticleSystem>();
    confettiParticles_->Initialize("resources/circle.png");
    confettiParticles_->SetBlend(BlendMode::kBlendModeAdd);
    confettiParticles_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    confettiParticles_->SetCull(PSOManager::CullMode::None);
    confettiParticles_->SetParticleLife(2.0f, 4.0f);
    confettiParticles_->SetParticleScale({0.5f, 0.5f, 0.5f}, {0.8f, 0.8f, 0.8f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
    // カラフルなパーティクル
    confettiParticles_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
    confettiParticles_->SetStartColor({1.0f, 0.8f, 0.2f, 1.0f}, {0.2f, 0.8f, 1.0f, 1.0f});
    confettiParticles_->SetEndColor({1.0f, 0.2f, 0.5f, 0.0f}, {0.5f, 1.0f, 0.2f, 0.0f});
    confettiParticles_->SetGravity(0.3f); // HLSLではプラスが下方向への重力。0.3f程度が適正
    confettiParticles_->SetDamping(0.01f); // 空気抵抗
    confettiParticles_->SetEmit(false);

    // 文字の初期配置（上空）
    const float clearPositionsX[6] = { -3.5f, -2.1f, -0.7f, 0.7f, 2.1f, 3.5f };
    ObjClass* clearTexts[6] = {
        clearTextC_.get(), clearTextL_.get(), clearTextE_.get(),
        clearTextA_.get(), clearTextR_.get(), clearTextEx_.get()
    };
    for (int i = 0; i < 6; ++i) {
        if (clearTexts[i]) {
            clearTexts[i]->SetPosition({ clearPositionsX[i], 15.0f, 0.0f });
        }
    }
}

void ClearScene::Update() {

    BaseScene::Update();

    // =====
    // ↓ゲームの更新
    // =====

    // Clear文字のアニメーション（ポップなバウンド）
    clearTextAnimator_.Update(1.0f / 60.0f);
    const float clearBaseY = 1.0f;
    const float clearPositionsX[6] = { -3.5f, -2.1f, -0.7f, 0.7f, 2.1f, 3.5f };

    ObjClass* clearTexts[6] = {
        clearTextC_.get(), clearTextL_.get(), clearTextE_.get(),
        clearTextA_.get(), clearTextR_.get(), clearTextEx_.get()
    };

    if (isSlamming_) {
        // スラムダウン演出
        introTimer_ += 1.0f / 60.0f;
        
        // 0.2秒ごとに1文字ずつ落とす
        int targetIndex = static_cast<int>(introTimer_ / 0.15f);
        if (targetIndex > 5) targetIndex = 5;

        for (int i = 0; i <= targetIndex; ++i) {
            if (!clearTexts[i]) continue;
            
            Vector3 pos = clearTexts[i]->GetPosition();
            if (pos.y > clearBaseY) {
                pos.y -= 60.0f * (1.0f / 60.0f); // 猛スピードで落下
                if (pos.y <= clearBaseY) {
                    pos.y = clearBaseY;
                    
                    // 着弾時の処理
                    if (currentSlamIndex_ < i) {
                        currentSlamIndex_ = i;
                        engine_->GetCameraManager()->GetActiveCamera()->Shake(0.5f, 5); // 画面揺れ
                        
                        // 最後の文字が落ちたとき
                        if (i == 5) {
                            isSlamming_ = false;
                            clearTextAnimator_.Reset(); // バウンドアニメーション用タイマーリセット
                            
                            // ビーム状に上に向かって吹き出すエミッター
                            // frequency を 0.05f に設定し、毎秒2000個ペースで連続噴出させる
                            // spread が 45.0f だと接線方向へのベクトルが45倍になり光速で画面外に消えるため、1.0f（約45度の広がり）に修正
                            // HLSL側ではvelocityは「1フレームあたりの移動量」として処理されるため、20.0fから0.4fへ適正化
                            confettiParticles_->SetBeamEmitter({0.0f, -5.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 5.0f, 0.4f, 1.0f, 100, 0.05f);
                            confettiParticles_->SetEmit(true);
                        }
                    }
                }
                clearTexts[i]->SetPosition(pos);
            }
        }
    } else {
        // 全文字着弾後のポップなバウンド演出
        // 1秒経過したら、エミッターを切り替えて上空から降らせる演出に移行する
        if (clearTextAnimator_.GetTime() > 1.0f) {
            if (!isRainingConfetti_) {
                isRainingConfetti_ = true;
                // 上空（Y=12）の広い範囲（幅25, 奥行き10）から毎秒100個（0.1秒に10個）を生成
                confettiParticles_->SetBoxEmitter({0.0f, 12.0f, 0.0f}, {25.0f, 2.0f, 10.0f}, 10, 0.1f);
                // 真下へ向かって少しだけ初速をつける
                confettiParticles_->SetDirection({0.0f, -1.0f, 0.0f});
                confettiParticles_->SetVelocity(0.05f); // ゆるやかな初速
                confettiParticles_->SetSpread(0.1f);    // 広がりは少なく
                confettiParticles_->SetEmit(true);      // 発生を継続
            }
        }

        for (int i = 0; i < 6; ++i) {
            if (!clearTexts[i]) continue;
            
            // 文字ごとに位相をずらす（少し早めのテンポ）
            float phaseOffset = -i * 0.5f;
            float phase = clearTextAnimator_.GetTime() * 5.0f + phaseOffset;
            
            // 1. ポップに跳ねる（絶対値のサイン波でバウンド）
            float offsetY = std::abs(std::sin(phase)) * 0.8f;
            
            // 2. 左右に少しだけ揺らす
            float rotZ = std::cos(phase) * 0.15f;
            
            clearTexts[i]->SetPosition({ clearPositionsX[i], clearBaseY + offsetY, 0.0f });
            clearTexts[i]->SetRotate({ 0.0f, 0.0f, rotZ });
        }
    }

    if (confettiParticles_) confettiParticles_->Update();

    // プロンプトコントローラーの更新
    promptController_.Update(engine_->GetInputManager());

    if (promptController_.ShouldTransition()) {
        engine_->GetSceneManager()->TransitionTo("Title", SceneTransition::Type::Fade, 1.0f);
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

    if (confettiParticles_) confettiParticles_->Draw();

    promptController_.Draw();
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


