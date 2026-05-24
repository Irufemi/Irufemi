#include "TitleScene.h"

#include "Framework/SceneManager.h"
#include <cmath>

#include "Irufemi.h"

#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"
#include "Resource/Audio/AudioManager.h"
#include "Engine/Manager/PrimitiveManager.h"

// デストラクタ
TitleScene::~TitleScene() {
    // シーン破棄時に自身のポストプロセスのみを取り除く
    if (engine_ && engine_->GetPostProcessManager()) {
        auto* pp = engine_->GetPostProcessManager();
        pp->RemoveActiveMode(PostProcessMode::Vignette);
        pp->RemoveActiveMode(PostProcessMode::Bloom);
        pp->RemoveActiveMode(PostProcessMode::ToneMapping);
        pp->RemoveActiveMode(PostProcessMode::HSV);
        
        // GameScene等に影響が出ないよう、変更したパラメータをデフォルトに戻す
        pp->GetHSVParams().saturation = 0.0f;
        pp->GetBloomParams().intensity = 1.0f;
        pp->GetBloomParams().threshold = 0.8f;
        pp->GetVignetteParams().scale = 16.0f;
    }
}

// 初期化
void TitleScene::Initialize(IrufemiEngine* engine) {

    BaseScene::Initialize(engine);

    // タイトルシーンでは背景色を極めて暗いブルーにして、真っ暗すぎない奥深さを出す
    engine_->SetClearColor(0.0f, 0.02f, 0.06f, 1.0f);

    // ポストプロセスの有効化（スタイリッシュな演出）
    if (auto* pp = engine_->GetPostProcessManager()) {
        pp->AddActiveMode(PostProcessMode::Vignette);
        // タイトル画面をより華やかにするためBloomとHSVも有効化
        pp->AddActiveMode(PostProcessMode::Bloom);
        pp->AddActiveMode(PostProcessMode::HSV);
        pp->AddActiveMode(PostProcessMode::ToneMapping);
        
        // ビネットで四隅を少し落とし、中央のタイトルに視線誘導
        pp->GetVignetteParams().scale = 10.0f;
        
        // Bloomパラメータ（光を溢れさせて暗さをカバーしつつリッチにする）
        pp->GetBloomParams().intensity = 1.2f;
        pp->GetBloomParams().threshold = 0.5f;

        // HSVパラメータ（少し彩度を高めてサイバーブルーを鮮やかに）
        pp->GetHSVParams().saturation = 0.2f; // デフォルト0.0から少し上げる

        // トーンマッピングでコントラストをパキッとさせる
        pp->GetToneMappingParams().exposure = 1.2f;
    }

    // シーン固有のカメラ位置に調整
    engine_->GetCameraManager()->GetActiveCamera()->SetTranslate({ 0.0f, 0.0f, -10.0f });
    engine_->GetCameraManager()->GetActiveCamera()->UpdateMatrix();

    // --- 3Dタイトル文字の初期化と配置 ---
    // 上段：「七転び」 (Y = 2.0, 少し左寄り)
    titleTextNana_ = std::make_unique<ObjClass>();
    titleTextNana_->Initialize("title/text/text_nana.obj");
    titleTextNana_->SetPosition({ -3.5f, 2.0f, 0.0f });
    titleTextNana_->SetScale({ 1.5f, 1.5f, 1.5f });

    titleTextKoro1_ = std::make_unique<ObjClass>();
    titleTextKoro1_->Initialize("title/text/text_koro.obj");
    titleTextKoro1_->SetPosition({ -0.5f, 2.0f, 0.0f });
    titleTextKoro1_->SetScale({ 1.5f, 1.5f, 1.5f });

    titleTextBi1_ = std::make_unique<ObjClass>();
    titleTextBi1_->Initialize("title/text/text_bi.obj");
    titleTextBi1_->SetPosition({ 2.5f, 2.0f, 0.0f });
    titleTextBi1_->SetScale({ 1.5f, 1.5f, 1.5f });

    // 下段：「八転び」 (Y = 0.0, 少し右寄り)
    titleTextHati_ = std::make_unique<ObjClass>();
    titleTextHati_->Initialize("title/text/text_hati.obj");
    titleTextHati_->SetPosition({ -2.5f, 0.0f, 0.0f });
    titleTextHati_->SetScale({ 1.5f, 1.5f, 1.5f });

    titleTextKoro2_ = std::make_unique<ObjClass>();
    titleTextKoro2_->Initialize("title/text/text_koro.obj");
    titleTextKoro2_->SetPosition({ 0.5f, 0.0f, 0.0f });
    titleTextKoro2_->SetScale({ 1.5f, 1.5f, 1.5f });

    titleTextBi2_ = std::make_unique<ObjClass>();
    titleTextBi2_->Initialize("title/text/text_bi.obj");
    titleTextBi2_->SetPosition({ 3.5f, 0.0f, 0.0f });
    titleTextBi2_->SetScale({ 1.5f, 1.5f, 1.5f });

    // 「Push to Space」文字
    titleTextPushToSpace_ = std::make_unique<ObjClass>();
    titleTextPushToSpace_->Initialize("text_pushtospace/text_pushtospace.obj");
    titleTextPushToSpace_->SetPosition({ 0.0f, -2.5f, 0.0f });
    titleTextPushToSpace_->SetScale({ 1.0f, 1.0f, 1.0f });
    
    // プロンプトコントローラーに登録
    promptController_.SetTarget(titleTextPushToSpace_.get());
    
    // --- 環境パーティクル（光の塵）の初期化 ---
    ambientParticles_ = std::make_unique<GPUParticleSystem>();
    ambientParticles_->Initialize("resources/circle.png");
    ambientParticles_->SetBlend(BlendMode::kBlendModeAdd);
    ambientParticles_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    ambientParticles_->SetCull(PSOManager::CullMode::None);
    // ランダムに画面全体に漂う設定
    ambientParticles_->SetParticleLife(2.0f, 4.0f);
    ambientParticles_->SetParticleScale({0.2f, 0.2f, 0.2f}, {0.3f, 0.3f, 0.3f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
    ambientParticles_->SetColor({1.0f, 1.0f, 1.0f, 0.5f});
    ambientParticles_->SetStartColor({1.0f, 1.0f, 0.8f, 0.8f}, {1.0f, 0.9f, 0.6f, 1.0f});
    ambientParticles_->SetEndColor({1.0f, 1.0f, 0.8f, 0.0f}, {1.0f, 0.9f, 0.6f, 0.0f});
    ambientParticles_->SetGravity(-0.02f); // マイナス重力で上にゆっくり浮上 (HLSLでは Y -= gravity*dt となるため、マイナスだと上へ加速)
    ambientParticles_->SetDamping(0.01f);
    
    // SetBoxEmitterのデフォルトでZ奥に飛んでいく（1.0f/フレーム = 秒速60）のを防ぐため、初速をほぼ0にする
    ambientParticles_->SetDirection({0.0f, 1.0f, 0.0f});
    ambientParticles_->SetVelocity(0.01f); 
    ambientParticles_->SetSpread(1.0f);
    
    ambientParticles_->SetEmit(true);

    // --- サイバー空間トンネルの初期化 ---
    tunnelObj_ = std::make_unique<PrimitiveObjects3DClass>();
    tunnelObj_->Initialize(PrimitiveType::Cylinder);
    
    // 蓋（トップ・ボトム）なしのシリンダーデータを生成して再初期化
    PrimitiveData noCapCylinder = PrimitiveManager::CreateCylinder(1.0f, 1.0f, 32, false, false);
    tunnelObj_->ReinitializeMesh(noCapCylinder);

    tunnelObj_->GetMaterial().enableLighting = false; // ライティング無効
    tunnelObj_->SetCastShadows(false);

    // CullMode::None にして内側からも見えるようにする（ユーザーの指摘通り）
    if (engine) {
        tunnelObj_->SetCustomPSO(engine->GetPSOManager()->GetPSO("CyberHex", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::None));
    }
    
    // Z軸に沿って寝かせる。長さを大幅に伸ばし、穴が見えないようにする
    Transform t;
    t.scale = { 50.0f, 1500.0f, 50.0f }; // 長さを1500に拡張
    t.rotate = { Math::PI / 2.0f, 0.0f, 0.0f }; // X軸で90度倒す
    t.translate = { 0.0f, 0.0f, 600.0f }; // 手前から奥までカバーするようにZ方向へずらす
    tunnelObj_->SetTransform(t);
    tunnelObj_->Update();

    // CyberHexParamsの初期化（GameSceneベースによりサイバー感強め）
    // コントラストをつけるため色を落ち着かせて、背景を極端に暗くする
    cyberHexParams_.edgeColor = { 0.0f, 0.4f, 0.7f, 1.0f }; // さらに渋めの青
    cyberHexParams_.edgeThickness = 0.08f;
    cyberHexParams_.baseBrightness = 0.02f; // 真っ黒すぎないように少しだけ明るく戻す
    cyberHexParams_.flickerAmplitude = 0.4f;
    cyberHexParams_.distortion = 0.05f;
    cyberHexParams_.density = 0.02f;        
    cyberHexParams_.animationSpeed = 0.2f;  
    cyberHexParams_.uvScrollX = 0.0f;
    cyberHexParams_.uvScrollY = -0.5f; // 手前から奥へゆっくり流れるようにYスクロール
    cyberHexParams_.mappingMode = 1.0f; // 円柱（トンネル）マッピングモードを有効化

    if (engine) {
        cyberHexCB_ = std::make_unique<DynamicConstantBuffer<CyberHexParams>>();
        cyberHexCB_->Initialize(engine->GetDirectXCommon(), 1);
        cyberHexCBIndex_ = cyberHexCB_->Allocate();
    }
}

void TitleScene::Finalize() {
    // 他のシーンに影響を与えないよう、クリアカラーをデフォルト（青みがかった色）に戻す
    engine_->SetClearColor(0.1f, 0.25f, 0.5f, 1.0f);
    BaseScene::Finalize();
}

// 更新
void TitleScene::Update() {

    BaseScene::Update(); // カメラ更新や定数バッファ送信などを自動化

    // =====
    // ↓ゲームの更新
    // =====

    globalTimer_ += 1.0f / 60.0f;

    if (cyberHexCB_) {
        // ダイブ中（開始時）はスクロール速度をグッと上げる
        if (isStarting_) {
            cyberHexParams_.uvScrollY -= 5.0f * (1.0f / 60.0f); 
        }
        uint32_t frameIndex = engine_->GetDirectXCommon()->GetFrameIndex();
        cyberHexCB_->Update(cyberHexCBIndex_, cyberHexParams_, frameIndex);
        if (tunnelObj_) {
            tunnelObj_->SetCustomCBVAddress(cyberHexCB_->GetGPUVirtualAddress(cyberHexCBIndex_, frameIndex));
        }
    }

    // 3D文字のアニメーション（ふわふわ浮遊）
    titleTextAnimator_.Update(1.0f / 60.0f);
    const float baseYTop = 2.0f;
    const float baseYBottom = 0.0f;
    const float basePositionsX[6] = { -3.5f, -0.5f, 2.5f, -2.5f, 0.5f, 3.5f };

    ObjClass* texts[6] = {
        titleTextNana_.get(), titleTextKoro1_.get(), titleTextBi1_.get(),
        titleTextHati_.get(), titleTextKoro2_.get(), titleTextBi2_.get()
    };

    for (int i = 0; i < 6; ++i) {
        if (!texts[i]) continue;

        // 文字ごとに位相（タイミング）をずらす
        float phaseOffset = i * 0.5f;

        // 1. ふわふわ浮遊 (Y軸の上下動)
        float offsetY = titleTextAnimator_.GetFloatOffset(0.2f, 2.0f, phaseOffset);
        float baseY = (i < 3) ? baseYTop : baseYBottom;
        texts[i]->SetPosition({ basePositionsX[i], baseY + offsetY, 0.0f });

        // 2. 回転はさせず、常に正面を向かせる
        texts[i]->SetRotate({ 0.0f, 0.0f, 0.0f });
    }

    // パーティクルの発生位置をカメラの前に設定してランダムに散らす
    auto cam = engine_->GetCameraManager()->GetActiveCamera();
    Vector3 camPos = cam->GetTranslate();
    ambientParticles_->SetBoxEmitter(camPos + Vector3{0.0f, 0.0f, 5.0f}, {15.0f, 10.0f, 5.0f}, 1, 0.1f);
    ambientParticles_->Update();

    // プロンプトコントローラーの更新（明滅、フラッシュ、キー入力待機）
    promptController_.Update(engine_->GetInputManager());

    // ゲーム開始演出（Juice）
    if (promptController_.IsDecided()) {
        if (!isStarting_) {
            isStarting_ = true;
        }

        startTimer_ += 1.0f / 60.0f;
        
        // カメラの高速ズームイン
        Vector3 pos = cam->GetTranslate();
        pos.z += 15.0f * (1.0f / 60.0f); // 毎フレーム手前に進む
        cam->SetTranslate(pos);

        // 文字の白フラッシュ演出
        float flash = (std::max)(0.0f, 1.0f - startTimer_ * 3.0f); // 急速に減衰
        Vector4 flashColor = { 1.0f + flash, 1.0f + flash, 1.0f + flash, 1.0f };
        for (int i = 0; i < 6; ++i) {
            if (texts[i]) texts[i]->SetColor(flashColor);
        }

    } else {
        // --- 待機中のカメラの揺らぎ（Sway） ---
        // ずっと同じ方向に回転すると文字が画面外に消えてしまうため、左右の揺れに変更
        cameraAngle_ += 0.5f * (1.0f / 60.0f);
        cam->SetTranslate({ 
            std::sin(cameraAngle_) * 1.0f, 
            std::cos(cameraAngle_ * 0.8f) * 0.5f, 
            -10.0f 
        });
        // 常に中央（文字）の方向をふんわり向くように
        cam->SetRotate({ 0.0f, std::sin(cameraAngle_) * 0.05f, 0.0f });
    }
    
    // 行列更新
    cam->UpdateMatrix();

    if (promptController_.ShouldTransition()) {
        engine_->GetSceneManager()->TransitionTo("InGame", SceneTransition::Type::Slide, 1.0f);
    }


    // =====
    // ↑ゲームの更新
    // =====
}

void TitleScene::Draw() {

    // --- トンネル背景の描画 ---
    if (tunnelObj_) {
        tunnelObj_->Draw();
    }

    // --- 3Dタイトル文字描画 ---
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);

    auto drawWithShadow = [](ObjClass* obj) {
        if (!obj) return;
        
        Vector3 pos = obj->GetPosition();
        Vector4 origColor = obj->GetColor();
        
        // 1. 黒い影を少し奥＆右下にずらして描画
        obj->SetPosition({ pos.x + 0.1f, pos.y - 0.1f, pos.z + 0.3f });
        obj->SetColor({ 0.0f, 0.0f, 0.0f, 0.8f });
        obj->SetEnableLightingOverride(0); // 影なので黒ベタ塗り
        obj->Draw();
        
        // 2. 本体の描画
        obj->SetPosition(pos);
        obj->SetColor(origColor);
        obj->SetEnableLightingOverride(-1);
        obj->Draw();
    };

    drawWithShadow(titleTextNana_.get());
    drawWithShadow(titleTextKoro1_.get());
    drawWithShadow(titleTextBi1_.get());
    drawWithShadow(titleTextHati_.get());
    drawWithShadow(titleTextKoro2_.get());
    drawWithShadow(titleTextBi2_.get());

    // パーティクルの描画 (Zバッファ無効・加算合成など)
    if (ambientParticles_) ambientParticles_->Draw();
    
    // PushToSpace も同様にコントローラ経由でなく直接影付きで描画...したいが
    // promptController_ が内部で Draw を呼ぶので、その前に影だけ描画する
    if (titleTextPushToSpace_) {
        Vector3 pos = titleTextPushToSpace_->GetPosition();
        Vector4 origColor = titleTextPushToSpace_->GetColor();
        
        titleTextPushToSpace_->SetPosition({ pos.x + 0.1f, pos.y - 0.1f, pos.z + 0.3f });
        titleTextPushToSpace_->SetColor({ 0.0f, 0.0f, 0.0f, 0.8f * origColor.w }); // アルファは元の明滅に合わせる
        titleTextPushToSpace_->SetEnableLightingOverride(0);
        titleTextPushToSpace_->Draw();
        
        titleTextPushToSpace_->SetPosition(pos);
        titleTextPushToSpace_->SetColor(origColor);
        titleTextPushToSpace_->SetEnableLightingOverride(-1);
    }
    promptController_.Draw();

}

void TitleScene::DrawDebugTab() {
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
