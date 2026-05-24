#include "PauseScene.h"
#include "Engine/IrufemiEngine.h"
#include "Framework/SceneManager.h"
#include "Renderer/Object2D/Sprite/Sprite.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"

PauseScene::PauseScene() {}

PauseScene::~PauseScene() {}

void PauseScene::Initialize(IrufemiEngine* engine) {
    // BaseScene::Initialize() を呼ぶと、GameScene のカメラやライトが新規生成されたデフォルトのもので
    // 上書きされてしまい、フラスタムカリングによりプレイヤーが消えてしまうため、ポインタの代入のみ行う。
    engine_ = engine;

    // --- ポーズメニューの初期化 ---
    pauseBgDimmerSprite_ = std::make_unique<Sprite>();
    pauseBgDimmerSprite_->Initialize("resources/whiteTexture.png");
    pauseBgDimmerSprite_->SetSize(static_cast<float>(engine_->GetClientWidth()), static_cast<float>(engine_->GetClientHeight()));
    pauseBgDimmerSprite_->SetColor(Vector4{0.1f, 0.1f, 0.1f, 0.6f});

    pauseTitleSprite_ = std::make_unique<Sprite>();
    pauseTitleSprite_->Initialize("resources/texture/pause/text_pausemenu.png");
    pauseTitleSprite_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() * 0.3f);

    pauseBackGameSprite_ = std::make_unique<Sprite>();
    pauseBackGameSprite_->Initialize("resources/texture/pause/text_backgame.png");
    pauseBackGameSprite_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() * 0.5f);

    pauseBackTitleSprite_ = std::make_unique<Sprite>();
    pauseBackTitleSprite_->Initialize("resources/texture/pause/text_backtitle.png");
    pauseBackTitleSprite_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() * 0.65f);

    // UISelectionGroup にメニュー項目を登録
    pauseMenuSelection_.AddItem(pauseBackGameSprite_.get());
    pauseMenuSelection_.AddItem(pauseBackTitleSprite_.get());
    pauseMenuSelection_.SetActiveBaseColor({1.0f, 1.0f, 1.0f, 1.0f});
    pauseMenuSelection_.SetInactiveColor({0.3f, 0.3f, 0.3f, 0.9f});
}

void PauseScene::Update() {
    // 自身でのライトやカメラの更新は行わない（下のシーンのものをそのまま使う）
    
    // ウィンドウリサイズに対応するためのUI動的配置
    float screenW = static_cast<float>(engine_->GetClientWidth());
    float screenH = static_cast<float>(engine_->GetClientHeight());
    float uiScale = screenH / 720.0f;
    
    if (pauseBgDimmerSprite_) pauseBgDimmerSprite_->SetSize(screenW, screenH);
    if (pauseTitleSprite_) {
        pauseTitleSprite_->SetUIScale(uiScale);
        pauseTitleSprite_->SetPositionCenter(screenW / 2.0f, screenH * 0.3f);
    }
    if (pauseBackGameSprite_) {
        pauseBackGameSprite_->SetUIScale(uiScale);
        pauseBackGameSprite_->SetPositionCenter(screenW / 2.0f, screenH * 0.5f);
    }
    if (pauseBackTitleSprite_) {
        pauseBackTitleSprite_->SetUIScale(uiScale);
        pauseBackTitleSprite_->SetPositionCenter(screenW / 2.0f, screenH * 0.65f);
    }

    // UISelectionGroup の更新
    pauseMenuSelection_.Update(engine_->GetInputManager());

    // 決定キーが押された場合の処理
    if (pauseMenuSelection_.IsDecided()) {
        if (pauseMenuSelection_.GetSelectedIndex() == 0) {
            // ゲームに戻る
            engine_->GetSceneManager()->PopScene();
            return;
        } else if (pauseMenuSelection_.GetSelectedIndex() == 1) {
            // タイトルに戻る (ポーズを解除してから遷移)
            // ※ PopSceneを呼ぶと自身(this)が破棄されるため、事前にポインタを退避しておく
            auto* sceneManager = engine_->GetSceneManager();
            sceneManager->PopScene();
            sceneManager->TransitionTo("Title", SceneTransition::Type::Fade, 1.0f);
            return;
        }
    }

    // ESC または STARTボタンでもポーズ解除可能にする
    InputManager* input = engine_->GetInputManager();
    if (input && (input->IsKeyPressed(VK_ESCAPE) || input->StartPressed())) {
        engine_->GetSceneManager()->PopScene();
        return;
    }

    // スプライト行列の更新
    if (pauseBgDimmerSprite_) pauseBgDimmerSprite_->Update();
    if (pauseTitleSprite_) pauseTitleSprite_->Update();
    if (pauseBackGameSprite_) pauseBackGameSprite_->Update();
    if (pauseBackTitleSprite_) pauseBackTitleSprite_->Update();
}

void PauseScene::Draw() {
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplyPSO("Sprite");

    // 背景ディマー描画
    if (pauseBgDimmerSprite_) pauseBgDimmerSprite_->Draw();

    if (pauseTitleSprite_) pauseTitleSprite_->Draw();

    // 選択項目の描画
    pauseMenuSelection_.Draw();
}
