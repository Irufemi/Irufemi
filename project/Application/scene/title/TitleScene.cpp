#include "TitleScene.h"

#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"
#include "scene/SceneManager.h"
#include <cmath> // sinf を使うために追加

// 初期化
void TitleScene::Initialize(IrufemiEngine* engine) {

    engine_ = engine;

    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f, 0.0f, -10.0f });

    // 重要：SetTranslate の後で行列を確実に更新しておく
    camera_->UpdateMatrix();

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(),
        engine_->GetClientWidth(),
        engine_->GetClientHeight());
    debugMode = false;

    pointLight_ = std::make_unique<PointLightClass>();
    pointLight_->Initialize();
    pointLight_->SetPos(Vector3{ 0.0f, -5.0f, 0.0f });
    engine_->GetDrawManager()->SetPointLightClass(pointLight_.get());

    spotLight_ = std::make_unique<SpotLightClass>();
    spotLight_->Initialize();
    spotLight_->SetIntensity(0.0f);
    engine_->GetDrawManager()->SetSpotLightClass(spotLight_.get());

    fade_.Initialize(engine_, camera_.get());
    fade_.StartFadeIn(0.5f);

    // タイトル文字の初期化
    titleText_ = std::make_unique<Sprite>();
    titleText_->Initialize(camera_.get(), "resources/title_titleTextFile.png");
    titleText_->SetAnchor(0.0f, 0.0f);
    titleText_->SetPosition(0.0f, 0.0f);
    titleText_->SetSize(1280.0f, 720.0f);
    titleText_->Update();


    pushText_ = std::make_unique<Sprite>();
    pushText_->Initialize(camera_.get(), "resources/title_pushText.png");
    pushText_->SetAnchor(0.0f, 0.0f);
    pushText_->SetPosition(0.0f, 0.0f);
    pushText_->SetSize(1280.0f, 720.0f);
    pushText_->Update();

    //SEの初期化
    cursolSE_.Initialize("resources/se/cursol.mp3");
    decisionSE_.Initialize("resources/se/decision.mp3");

    //タイトルBGMの初期化
    titleBGM_.Initialize("resources/bgm/titleBGM.mp3");
    titleBGM_.PlayFixed();

    deciding_ = false;
    decideTimer_ = 0.0f;
    idleAnimTimer_ = 0.0f;
}

// 更新
void TitleScene::Update() {
    const float deltaTime = 1.0f / 60.0f;

    // カメラの通常更新
    if (debugMode) {
        debugCamera_->Update();
        camera_->SetViewMatrix(debugCamera_->GetCamera().GetViewMatrix());
        camera_->SetPerspectiveFovMatrix(
            debugCamera_->GetCamera().GetPerspectiveFovMatrix());
    } else {
        camera_->Update("Camera", { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
    }

    // タイトル文字の更新
    titleText_->Update();

    // --- UIアニメーション処理 ---
    if (deciding_) {
        // 決定後の短い明滅アニメーション
        decideTimer_ += deltaTime;
        const float blinkDuration = 0.3f; // 明滅アニメーションの総時間
        const int blinkCount = 2; // 明滅回数

        if (decideTimer_ < blinkDuration) {
            // 1回の明滅（消灯→点灯）にかかる時間
            float singleBlinkTime = blinkDuration / blinkCount;
            // 現在の明滅サイクルの進捗 (0-1)
            float phase = fmodf(decideTimer_, singleBlinkTime) / singleBlinkTime;
            // 0.5を境に表示/非表示を切り替え
            pushText_->SetAlpha(phase < 0.5f ? 0.0f : 1.0f);
        } else {
            // アニメーション終了
            pushText_->SetAlpha(0.0f); // 最後に非表示にする
            // 次に行くシーン名をセットして、フェードアウト開始
            nextSceneName_ = "InGame";
            fade_.StartFadeOut(0.5f);
            deciding_ = false; // アニメーションとフェード開始処理を一度だけ行う
        }

    } else if (!fade_.IsFading()) {
        // アイドリング中のゆっくりとした明滅
        idleAnimTimer_ += deltaTime;
        const float blinkSpeed = 1.5f; // 明滅の速さ (2.0f -> 1.5f に変更して滑らかに)
        // sin波を使って 0.3 ～ 1.0 の範囲でアルファ値を変化させる
        float alpha = (sinf(idleAnimTimer_ * blinkSpeed) + 1.0f) / 2.0f * 0.7f + 0.3f;
        pushText_->SetAlpha(alpha);

        // 入力受付
        if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE) ||
            engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
            deciding_ = true;
            decideTimer_ = 0.0f;
            decisionSE_.Play();
        }
    }

    // プッシュ文字の更新
    pushText_->Update();

    fade_.Update(deltaTime);

    if (!fade_.IsFading() && !nextSceneName_.empty()) {
        engine_->GetSceneManager()->Request(nextSceneName_.c_str());
        nextSceneName_.clear();
    }
}

void TitleScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyPSO();

    // 2D

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();


    // タイトル文字の描画
    titleText_->Draw();

    // プッシュ文字の描画
    if (pushText_->GetColor().w > 0.0f) {
        pushText_->Draw();
    }


    fade_.Draw();
}