#include "ClearScene.h"

#include "Framework/SceneManager.h"

#include "Irufemi.h"

#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"
#include "Resource/Audio/AudioManager.h"
#include "contents/skydome/Skydome.h"
#include "Engine/Graphics/PostProcess/PostProcessManager.h"

ClearScene::~ClearScene() {
    // シーン破棄時に自身のポストプロセスのみを取り除く
    if (engine_ && engine_->GetPostProcessManager()) {
        auto* pp = engine_->GetPostProcessManager();
        pp->RemoveActiveMode(PostProcessMode::Bloom);
        pp->RemoveActiveMode(PostProcessMode::RadialBlur);
        
        // 他のシーンに影響が出ないよう、変更したパラメータをデフォルトに戻す
        pp->GetRadialBlurParams().blurWidth = 0.0f;
    }
}

void ClearScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    // ポストプロセスの有効化（華やか・祝祭感の演出）
    if (auto* pp = engine_->GetPostProcessManager()) {
        pp->AddActiveMode(PostProcessMode::Bloom);
        pp->AddActiveMode(PostProcessMode::RadialBlur);
        
        pp->GetRadialBlurParams().center = { 0.5f, 0.5f };
        pp->GetRadialBlurParams().blurWidth = 0.0f; // スラムダウン時に値を上げる
    }

    // Skydomeの初期化
    skydome_ = std::make_unique<Skydome>();
    skydome_->Initialize();
    // 明るすぎず暗すぎないベストな明度（1.3倍）で、文字の視認性と祝祭感を両立
    skydome_->SetColor({ 1.3f, 1.3f, 1.3f, 1.0f });

    // シーン固有のカメラ位置・向きにリセット
    engine_->GetCameraManager()->GetActiveCamera()->SetTranslate({ 0.0f, 0.0f, -10.0f });
    engine_->GetCameraManager()->GetActiveCamera()->SetRotate({ 0.0f, 0.0f, 0.0f });
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

    // 選択肢（やりなおし、タイトルへ戻る）の初期化
    objRetry_ = std::make_unique<ObjClass>();
    objRetry_->Initialize("text_retry/text_retry.obj");
    objRetry_->SetPosition({ -3.0f, -2.5f, 0.0f });
    objRetry_->SetScale({ 1.0f, 1.0f, 1.0f });
    objRetry_->SetEnableLightingToAllMeshes(false); // ★ ライティングの影響を受けず常に明るく発色させる

    objBackToTitle_ = std::make_unique<ObjClass>();
    objBackToTitle_->Initialize("text_backToTitle/text_backToTitle.obj");
    objBackToTitle_->SetPosition({ 3.0f, -2.5f, 0.0f });
    objBackToTitle_->SetScale({ 1.0f, 1.0f, 1.0f });
    objBackToTitle_->SetEnableLightingToAllMeshes(false); // ★ ライティングの影響を受けず常に明るく発色させる

    // UISelectionGroupの設定
    clearSelection_.SetHorizontalMode(true);
    clearSelection_.AddItem(objRetry_.get());
    clearSelection_.AddItem(objBackToTitle_.get());
    clearSelection_.SetActiveBaseColor({ 1.5f, 1.2f, 0.3f, 1.0f }); // 発光させるため1.0を超える値（Bloomが強くかかる）
    clearSelection_.SetInactiveColor({ 0.9f, 0.8f, 0.4f, 1.0f }); // 非選択時も暗くならず、はっきり読めるゴールドに

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

    // 花火パーティクルの初期化
    fireworksParticles_ = std::make_unique<GPUParticleSystem>();
    fireworksParticles_->Initialize("resources/circle.png");
    fireworksParticles_->SetBlend(BlendMode::kBlendModeAdd);
    fireworksParticles_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    fireworksParticles_->SetCull(PSOManager::CullMode::None);
    fireworksParticles_->SetParticleLife(1.0f, 2.0f);
    fireworksParticles_->SetParticleScale({0.2f, 0.2f, 0.2f}, {0.4f, 0.4f, 0.4f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
    fireworksParticles_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
    fireworksParticles_->SetStartColor({1.5f, 1.0f, 0.5f, 1.0f}, {0.5f, 1.5f, 1.5f, 1.0f});
    fireworksParticles_->SetEndColor({1.0f, 0.2f, 0.5f, 0.0f}, {0.5f, 1.0f, 0.2f, 0.0f});
    fireworksParticles_->SetGravity(-0.02f); // バースト後に少し下に落ちる（重力）
    fireworksParticles_->SetDamping(0.15f); // 空気抵抗で急激に減速させて「弾ける」感を出す
    fireworksParticles_->SetEmit(false);

    // 文字の初期配置（上空）
    const float clearPositionsX[6] = { -3.5f, -2.1f, -0.7f, 0.7f, 2.1f, 3.5f };
    ObjClass* clearTexts[6] = {
        clearTextC_.get(), clearTextL_.get(), clearTextE_.get(),
        clearTextA_.get(), clearTextR_.get(), clearTextEx_.get()
    };
    for (int i = 0; i < 6; ++i) {
        if (clearTexts[i]) {
            clearTexts[i]->SetPosition({ clearPositionsX[i], 15.0f, 0.0f });
            clearTexts[i]->SetEnableLightingToAllMeshes(false); // ★ ライティングを切って自ら発光（Bloom）させる
        }
    }
}

void ClearScene::Update() {

    BaseScene::Update();

    if (skydome_) {
        skydome_->AddRotateY(0.05f * (1.0f / 60.0f)); // 空をゆっくり回して幻想的に
        skydome_->Update();
    }

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
        // スラムダウン演出中はラジアルブラーをかける
        if (auto* pp = engine_->GetPostProcessManager()) {
            pp->GetRadialBlurParams().blurWidth = 0.02f;
        }

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
                            
                            // ラジアルブラーを戻す
                            if (auto* pp = engine_->GetPostProcessManager()) {
                                pp->GetRadialBlurParams().blurWidth = 0.0f;
                            }
                            
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
        // カメラのゆるやかなスウェイ（揺れ）演出
        // 可読性を保つため、完全な旋回ではなく左右への緩やかな移動（パララックス効果）に留める
        cameraAngle_ = std::sin(clearTextAnimator_.GetTime() * 0.5f) * 0.15f; // 最大で約8.5度ほどの傾き
        engine_->GetCameraManager()->GetActiveCamera()->SetTranslate({ std::sin(cameraAngle_) * 10.0f, 0.0f, -std::cos(cameraAngle_) * 10.0f });
        engine_->GetCameraManager()->GetActiveCamera()->SetRotate({ 0.0f, -cameraAngle_, 0.0f });
        engine_->GetCameraManager()->GetActiveCamera()->UpdateMatrix();

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

        // 花火の打ち上げ
        if (isRainingConfetti_) {
            fireworksTimer_ += 1.0f / 60.0f;
            if (fireworksTimer_ >= 0.5f) { // 0.5秒おきに
                fireworksTimer_ = 0.0f;
                // 画面のランダムな位置（X: -15 ~ 15, Y: 0 ~ 10, Z: 5）
                float randX = (rand() % 300 - 150) * 0.1f;
                float randY = (rand() % 150) * 0.1f; // 少し高めまで
                
                // バーストごとのランダムカラー生成（明るめ）
                Vector4 c1 = { (rand() % 100) * 0.01f + 0.5f, (rand() % 100) * 0.01f + 0.5f, (rand() % 100) * 0.01f + 0.5f, 1.0f };
                Vector4 c2 = { (rand() % 100) * 0.01f + 0.5f, (rand() % 100) * 0.01f + 0.5f, (rand() % 100) * 0.01f + 0.5f, 1.0f };
                fireworksParticles_->SetStartColor(c1, c2);

                // 爆発の勢い（Velocity）と拡散（Spread）を設定して本当に弾けるようにする
                fireworksParticles_->SetSphereEmitter({randX, randY, 8.0f}, 0.2f, 80, 0.0f); // 小さな点から80個
                fireworksParticles_->SetSpread(1.0f); // 放射状に100%広がる
                fireworksParticles_->SetVelocity(0.3f + (rand() % 100) * 0.002f); // 0.3〜0.5の強い初速（※HLSLでは1フレームの移動量）
                fireworksParticles_->Emit(80); // バースト
            }
        }

        // 煌めくゴールドカラー（Bloomに引っかかるように1.0以上の輝度を設定）
        float goldIntensity = 0.8f + std::sin(clearTextAnimator_.GetTime() * 5.0f) * 0.2f;
        Vector4 goldColor = { 1.5f * goldIntensity, 1.2f * goldIntensity, 0.3f * goldIntensity, 1.0f };

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
            clearTexts[i]->SetRotate({ 0.0f, 0.0f, rotZ }); // 文字群としてのまとまり（可読性）を保つため正面を固定
            clearTexts[i]->SetColor(goldColor);
        }
    }

    if (confettiParticles_) confettiParticles_->Update();
    if (fireworksParticles_) fireworksParticles_->Update();

    // 選択肢コントローラーの表示・更新（文字が落ちきってから少し後）
    if (!isSlamming_ && clearTextAnimator_.GetTime() > 1.0f) {
        // 1秒かけてフェードイン
        float uiAlpha = std::clamp((clearTextAnimator_.GetTime() - 1.0f) * 1.0f, 0.0f, 1.0f);
        clearSelection_.SetActiveBaseColor({ 1.0f, 0.8f, 0.2f, 1.0f * uiAlpha });
        clearSelection_.SetInactiveColor({ 0.3f, 0.2f, 0.0f, 0.8f * uiAlpha });

        clearSelection_.Update(engine_->GetInputManager());

        if (clearSelection_.ShouldTransition()) {
            if (clearSelection_.GetSelectedIndex() == 0) {
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

void ClearScene::Draw() {
    // --- 3D描画 ---
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);

    if (skydome_) skydome_->Draw();

    ObjClass* clearTexts[6] = {
        clearTextC_.get(), clearTextL_.get(), clearTextE_.get(),
        clearTextA_.get(), clearTextR_.get(), clearTextEx_.get()
    };

    // 1パス目：黒色のドロップシャドウ
    for (int i = 0; i < 6; ++i) {
        if (!clearTexts[i]) continue;
        Vector3 pos = clearTexts[i]->GetPosition();
        Vector4 col = clearTexts[i]->GetColor(); // 色を退避
        
        // 影の位置と色（右下奥へオフセット）
        // カメラの旋回に合わせて影を落とす方向（ローカル空間）を計算
        Vector3 right = { std::cos(-cameraAngle_), 0.0f, std::sin(-cameraAngle_) };
        Vector3 forward = { std::sin(-cameraAngle_), 0.0f, -std::cos(-cameraAngle_) };
        Vector3 offset = right * 0.15f + Vector3{0.0f, -0.15f, 0.0f} + forward * 0.15f;

        clearTexts[i]->SetPosition(pos + offset);
        clearTexts[i]->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });
        clearTexts[i]->Draw();
        
        // 元に戻す
        clearTexts[i]->SetPosition(pos);
        clearTexts[i]->SetColor(col);
    }

    // 2パス目：本体の描画
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable); // 一応再セット
    for (int i = 0; i < 6; ++i) {
        if (clearTexts[i]) clearTexts[i]->Draw();
    }

    if (confettiParticles_) confettiParticles_->Draw();
    if (fireworksParticles_) fireworksParticles_->Draw();

    if (!isSlamming_ && clearTextAnimator_.GetTime() > 1.0f) {
        clearSelection_.Draw();
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


