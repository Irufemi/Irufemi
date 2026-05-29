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
#include "scene/inGame/GameScene.h"
#include <fstream>

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

    bgm_ = std::make_unique<Bgm>();
    bgm_->Initialize("resources/BGM/Clear.mp3", "ClearBGM", true, true);

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

    // 花火パーティクルの初期化（新システムへ移行・プール事前生成）
    for (int i = 0; i < 10; ++i) {
        fireworksPool_[i] = std::make_unique<FireworkEffect>();
        fireworksPool_[i]->Initialize(engine); // engine は Initialize の引数
    }

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

    // タイム表示スプライトの生成（遅延初期化用）
    timeMinutes10_ = std::make_unique<Sprite>();
    timeMinutes10_->SetAnchor(0.0f, 0.0f);

    timeMinutes1_ = std::make_unique<Sprite>();
    timeMinutes1_->SetAnchor(0.0f, 0.0f);

    timeColon_ = std::make_unique<Sprite>();
    timeColon_->SetAnchor(0.0f, 0.0f);

    timeSeconds10_ = std::make_unique<Sprite>();
    timeSeconds10_->SetAnchor(0.0f, 0.0f);

    timeSeconds1_ = std::make_unique<Sprite>();
    timeSeconds1_->SetAnchor(0.0f, 0.0f);

    // 最高記録表示スプライトの生成
    bestMinutes10_ = std::make_unique<Sprite>();
    bestMinutes10_->SetAnchor(0.0f, 0.0f);

    bestMinutes1_ = std::make_unique<Sprite>();
    bestMinutes1_->SetAnchor(0.0f, 0.0f);

    bestColon_ = std::make_unique<Sprite>();
    bestColon_->SetAnchor(0.0f, 0.0f);

    bestSeconds10_ = std::make_unique<Sprite>();
    bestSeconds10_->SetAnchor(0.0f, 0.0f);

    bestSeconds1_ = std::make_unique<Sprite>();
    bestSeconds1_->SetAnchor(0.0f, 0.0f);

    // タイトルスプライトの生成
    thisRecordTitleSprite_ = std::make_unique<Sprite>();
    thisRecordTitleSprite_->SetAnchor(0.5f, 0.5f);

    bestRecordTitleSprite_ = std::make_unique<Sprite>();
    bestRecordTitleSprite_->SetAnchor(0.5f, 0.5f);

    isTimeSpritesInitialized_ = false;

    // --- 最高記録の読み込みと更新 ---
    bestTime_ = 999999.0f;
    {
        std::ifstream ifs("best_time.txt");
        if (ifs) {
            ifs >> bestTime_;
        }
    }
    float clearTime = GameScene::GetClearTime();
    if (clearTime < bestTime_) {
        bestTime_ = clearTime;
        std::ofstream ofs("best_time.txt");
        if (ofs) {
            ofs << bestTime_;
        }
    }

    // タイム表示テクスチャの非同期ロード（登録）を開始しておく
    if (engine_ && engine_->GetTextureManager()) {
        engine_->GetTextureManager()->GetTextureHandle("resources/texture/inGame/numbers.png");
        engine_->GetTextureManager()->GetTextureHandle("resources/texture/inGame/colon.png");
        engine_->GetTextureManager()->GetTextureHandle("resources/texture/inGame/This record.png");
        engine_->GetTextureManager()->GetTextureHandle("resources/texture/inGame/best record.png");
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

        // 花火の打ち上げ（新システム）
        if (isRainingConfetti_) {
            fireworksTimer_ += 1.0f / 60.0f;
            if (fireworksTimer_ >= nextFireworkInterval_) { 
                fireworksTimer_ = 0.0f;
                
                // 次の花火までの間隔をランダムに決定して「情緒（タメと連発）」を演出
                int randVal = rand() % 100;
                if (randVal < 20) {
                    // 20%の確率で「スターマイン（連発）」: 0.1秒〜0.3秒間隔
                    nextFireworkInterval_ = 0.1f + (rand() % 21) * 0.01f;
                } else if (randVal < 35) {
                    // 15%の確率で「タメ」: 1.0秒〜1.8秒の静寂
                    nextFireworkInterval_ = 1.0f + (rand() % 81) * 0.01f;
                } else {
                    // 65%の確率で「通常」: 0.4秒〜0.7秒間隔
                    nextFireworkInterval_ = 0.4f + (rand() % 31) * 0.01f;
                }
                
                // 待機中または終了済みの花火を探して打ち上げる
                for (auto& firework : fireworksPool_) {
                    if (firework && (firework->IsWaiting() || firework->IsFinished())) {
                        // 打ち上げ位置は画面下部（文字のX幅に合わせた範囲）、目標は上空ランダム
                        float startX = (rand() % 160 - 80) * 0.1f; // -8.0 〜 8.0
                        float startY = -5.0f; // 画面外下
                        float targetY = (rand() % 30) * 0.1f + 2.0f; // 2.0〜5.0 (さらに低く)
                        float targetX = startX + (rand() % 40 - 20) * 0.1f; // 少し左右にブレる
                        
                        // 文字の少し奥(Z=8.0)で打ち上げる
                        firework->Fire({startX, startY, 8.0f}, {targetX, targetY, 8.0f}); 
                        break;
                    }
                }
            }
            
            // 花火の更新と終了判定
            for (auto& firework : fireworksPool_) {
                if (firework && !firework->IsWaiting()) {
                    firework->Update(1.0f / 60.0f);
                    if (firework->IsFinished()) {
                        firework->Reset();
                    }
                }
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

    // 選択肢コントローラーの表示・更新（文字が落ちきってから少し後）
    if (!isSlamming_ && clearTextAnimator_.GetTime() > 1.0f) {
        // 1秒かけてフェードイン
        float uiAlpha = std::clamp((clearTextAnimator_.GetTime() - 1.0f) * 1.0f, 0.0f, 1.0f);
        clearSelection_.SetActiveBaseColor({ 1.0f, 0.8f, 0.2f, 1.0f * uiAlpha });
        clearSelection_.SetInactiveColor({ 0.3f, 0.2f, 0.0f, 0.8f * uiAlpha });

        // タイム表示スプライトの遅延初期化（サイズが確定するのを待つ）
        if (!isTimeSpritesInitialized_ && engine_ && engine_->GetTextureManager()) {
            uint32_t tw = 0, th = 0;
            uint32_t cw = 0, ch = 0;
            uint32_t t1w = 0, t1h = 0;
            uint32_t t2w = 0, t2h = 0;
            if (engine_->GetTextureManager()->GetTextureSize("resources/texture/inGame/numbers.png", tw, th) && tw > 0 &&
                engine_->GetTextureManager()->GetTextureSize("resources/texture/inGame/colon.png", cw, ch) && cw > 0 &&
                engine_->GetTextureManager()->GetTextureSize("resources/texture/inGame/This record.png", t1w, t1h) && t1w > 0 &&
                engine_->GetTextureManager()->GetTextureSize("resources/texture/inGame/best record.png", t2w, t2h) && t2w > 0) {
                
                timeMinutes10_->Initialize("resources/texture/inGame/numbers.png");
                timeMinutes1_->Initialize("resources/texture/inGame/numbers.png");
                timeColon_->Initialize("resources/texture/inGame/colon.png");
                timeSeconds10_->Initialize("resources/texture/inGame/numbers.png");
                timeSeconds1_->Initialize("resources/texture/inGame/numbers.png");

                bestMinutes10_->Initialize("resources/texture/inGame/numbers.png");
                bestMinutes1_->Initialize("resources/texture/inGame/numbers.png");
                bestColon_->Initialize("resources/texture/inGame/colon.png");
                bestSeconds10_->Initialize("resources/texture/inGame/numbers.png");
                bestSeconds1_->Initialize("resources/texture/inGame/numbers.png");

                thisRecordTitleSprite_->Initialize("resources/texture/inGame/This record.png");
                bestRecordTitleSprite_->Initialize("resources/texture/inGame/best record.png");

                isTimeSpritesInitialized_ = true;
            }
        }

        // --- クリアタイム表示の更新 ---
        if (isTimeSpritesInitialized_) {
            float clearTime = GameScene::GetClearTime();
            
            auto getDigits = [](float timeVal, int& m10, int& m1, int& s10, int& s1) {
                int minutes = static_cast<int>(timeVal) / 60;
                int seconds = static_cast<int>(timeVal) % 60;
                if (minutes > 99) {
                    minutes = 99;
                    seconds = 59;
                }
                m10 = minutes / 10;
                m1 = minutes % 10;
                s10 = seconds / 10;
                s1 = seconds % 10;
            };

            int thisM10, thisM1, thisS10, thisS1;
            getDigits(clearTime, thisM10, thisM1, thisS10, thisS1);

            int bestM10, bestM1, bestS10, bestS1;
            getDigits(bestTime_, bestM10, bestM1, bestS10, bestS1);

            auto setDigitRect = [](Sprite* sprite, int digit) {
                if (sprite) {
                    sprite->SetTextureRectPixels(digit * 40, 0, 40, 40, false);
                }
            };

            setDigitRect(timeMinutes10_.get(), thisM10);
            setDigitRect(timeMinutes1_.get(), thisM1);
            setDigitRect(timeSeconds10_.get(), thisS10);
            setDigitRect(timeSeconds1_.get(), thisS1);

            setDigitRect(bestMinutes10_.get(), bestM10);
            setDigitRect(bestMinutes1_.get(), bestM1);
            setDigitRect(bestSeconds10_.get(), bestS10);
            setDigitRect(bestSeconds1_.get(), bestS1);

            float screenW = static_cast<float>(engine_->GetClientWidth());
            float screenH = static_cast<float>(engine_->GetClientHeight());
            float uiScale = screenH / 720.0f;

            float charSize = 48.0f * uiScale;
            float spacing = 4.0f * uiScale;
            float stepX = charSize + spacing;
            float totalW = stepX * 4.0f + charSize;

            float centerX = screenW / 2.0f;
            float startY = 410.0f * uiScale; // ボタンと被らないように少し位置を調整

            // 左右の間隔
            float gap = 80.0f * uiScale;

            float startX_this = centerX - totalW - gap;
            float startX_best = centerX + gap;

            // ゴールド発光色（Bloomに引っかかるように1.0を超える輝度、uiAlphaをアルファに掛ける）
            float intensity = 0.8f + std::sin(clearTextAnimator_.GetTime() * 5.0f) * 0.2f;
            Vector4 goldColor = { 1.5f * intensity, 1.2f * intensity, 0.3f * intensity, uiAlpha };

            auto updateTimeSprites = [&](Sprite* m10, Sprite* m1, Sprite* colon, Sprite* s10, Sprite* s1, float startX) {
                auto updateSprite = [&](Sprite* sprite, float x, float y, float size) {
                    if (sprite) {
                        sprite->SetSize(size, size);
                        sprite->SetPositionTopLeft(x, y);
                        sprite->SetColor(goldColor);
                        sprite->Update();
                    }
                };
                updateSprite(m10, startX, startY, charSize);
                updateSprite(m1, startX + stepX, startY, charSize);
                updateSprite(colon, startX + stepX * 2.0f, startY, charSize);
                updateSprite(s10, startX + stepX * 3.0f, startY, charSize);
                updateSprite(s1, startX + stepX * 4.0f, startY, charSize);
            };

            updateTimeSprites(timeMinutes10_.get(), timeMinutes1_.get(), timeColon_.get(), timeSeconds10_.get(), timeSeconds1_.get(), startX_this);
            updateTimeSprites(bestMinutes10_.get(), bestMinutes1_.get(), bestColon_.get(), bestSeconds10_.get(), bestSeconds1_.get(), startX_best);

            // タイトル画像の更新
            auto updateTitleSprite = [&](Sprite* sprite, float targetStartX) {
                if (sprite) {
                    float tW = 200.0f * uiScale;
                    float tH = 50.0f * uiScale;
                    float cX = targetStartX + totalW / 2.0f;
                    float cY = startY - 40.0f * uiScale; // タイム表示の上
                    sprite->SetSize(tW, tH);
                    sprite->SetPositionCenter(cX, cY);
                    sprite->SetColor(goldColor);
                    sprite->Update();
                }
            };

            updateTitleSprite(thisRecordTitleSprite_.get(), startX_this);
            updateTitleSprite(bestRecordTitleSprite_.get(), startX_best);
        }

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
    for (auto& firework : fireworksPool_) {
        if (firework && !firework->IsWaiting()) {
            firework->Draw();
        }
    }

    if (!isSlamming_ && clearTextAnimator_.GetTime() > 1.0f) {
        clearSelection_.Draw();

        // タイム表示スプライトの描画
        if (isTimeSpritesInitialized_) {
            if (timeMinutes10_) timeMinutes10_->Draw();
            if (timeMinutes1_) timeMinutes1_->Draw();
            if (timeColon_) timeColon_->Draw();
            if (timeSeconds10_) timeSeconds10_->Draw();
            if (timeSeconds1_) timeSeconds1_->Draw();

            if (bestMinutes10_) bestMinutes10_->Draw();
            if (bestMinutes1_) bestMinutes1_->Draw();
            if (bestColon_) bestColon_->Draw();
            if (bestSeconds10_) bestSeconds10_->Draw();
            if (bestSeconds1_) bestSeconds1_->Draw();

            if (thisRecordTitleSprite_) thisRecordTitleSprite_->Draw();
            if (bestRecordTitleSprite_) bestRecordTitleSprite_->Draw();
        }
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


