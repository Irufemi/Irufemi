#include "GameScene.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"

#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "math/CameraForGPU.h"
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/DirectionalLight.h"
#include "math/AreaLight.h"
#include "2D/Sprite.h"

#include "function/Collision.h"
#include "function/Math.h"
#include "function/Ease.h"

#include "Application/scene/stageSelect/StageDataManager.h"
#include "Application/scene/result/GameResultManager.h"

// デストラクタ
GameScene::~GameScene() {

}

// 初期化
void GameScene::Initialize(IrufemiEngine* engine) {
    // エンジンへのポインタを保持
    engine_ = engine;

    // 各コンポーネントの初期化
    InitializeSystem();
    InitializeGameObjects();
    InitializeUI();

    // フェーズとBGMの初期化
    phase_ = Phase::FadeIn;
    bgm_ = std::make_unique<Bgm>();
    bgm_->Initialize("resources/bgm/ingame.mp3");
    bgm_->PlayFixed();
}

// 更新
void GameScene::Update() {

#if defined USE_IMGUI

    ImGui::Begin("GameScene");
    if (ImGui::BeginTabBar("GameSceneTabs")) {

        DebugUI::DebugLights(directionalLight_.get(), pointLights_, spotLights_, areaLights_);

        // Texture タブ
        if (ImGui::BeginTabItem("Texture")) {
            if (ImGui::Button("allLoadActivate")) {
                engine_->GetTextureManager()->LoadAllFromFolder("resources/");
            }
            ImGui::EndTabItem();
        }

        // Debug タブ
        if (ImGui::BeginTabItem("Debug")) {
            ImGui::Checkbox("debugMode", &debugMode_);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();

#endif // _DEBUG

    // --- カメラの更新 ---
    // 現在アクティブなカメラへのポインタ
    Camera* currentCamera = debugMode_ ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera"); // デバッグカメラも通常カメラもUpdateを呼ぶ

    // 天球の更新
    skydome_->Update();

    manual_->Update();

    // フェードの更新
    fade_->Update();

    // =====
    // ↓ゲームの更新
    // =====

    // フェーズごとの更新
    switch (phase_) {
    case Phase::FadeIn:
        UpdateFadeIn();
        break;
    case Phase::Countdown:
        UpdateCountdown();
        break;
    case Phase::Gameplay:
        UpdateGameplay();
        break;
    case Phase::FadeOut:
        UpdateFadeOut();
        break;
    }

    // デバッグモードでない場合のみ、カメラコントローラーを適用
    if (!debugMode_) {
        cameraController_->Update(*camera_.get());
    }

    // =====
    // ↑ゲームの更新
    // =====

    // --- フレーム共通データのセット ---
    CameraForGPU cameraForGpu;
    cameraForGpu.view = currentCamera->GetViewMatrix();
    cameraForGpu.projection = currentCamera->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = currentCamera->GetTranslate();

    std::vector<PointLight*> pLights;
    for (const auto& light : pointLights_) {
        pLights.push_back(light.get());
    }
    std::vector<SpotLight*> sLights;
    for (const auto& light : spotLights_) {
        sLights.push_back(light.get());
    }
    std::vector<AreaLight*> aLights;
    for (const auto& light : areaLights_) {
        aLights.push_back(light.get());
    }

    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, pLights, sLights, aLights);
}

// 描画
void GameScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyPSO();

    // 天球
    skydome_->Draw();

    // Player
    player_->Draw();

    // 敵
    for (const auto& enemy : enemies_) {
        enemy->Draw();
    }

    // Region
    engine_->ApplyRegionPSO();

    // ブロック
    blocks_->Draw();

    // Sprite
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    // UI

    // HP
    text_HP_->Draw();

    // HUD

    // HPBar
    // out
    hpBar_out_->Draw();
    // in
    hpBar_in_->Draw();

    manual_->Draw();

    // カウントダウンUIの描画
    if (phase_ == Phase::Countdown) {
        countdownText_killEnemy_->Draw();
        if (countdownTimer_ > 2.0f) {
            text_3_->Draw();
        } else if (countdownTimer_ > 1.0f) {
            text_2_->Draw();
        } else if (countdownTimer_ > 0.0f) {
            text_1_->Draw();
        }
    }

    // フェードの描画(最前面)
    fade_->Draw();
}

void GameScene::PauseUpdate()
{
    const float BLINK_SPEED = 4.0f; // 通常の明滅速度
    const float CONFIRM_BLINK_SPEED = 15.0f; // 決定時の高速な明滅速度
    const float CONFIRMATION_DURATION = 0.5f; // 決定演出の時間

    // ポーズ中の更新処理
    if (pauseSprite_) {
        pauseSprite_->Update();
    }

    // メニュー選択中の処理
    if (pauseMenuState_ == PauseMenuState::Selecting) {
        // 入力による選択項目の変更
        if (IScene::PressedVK('W') || engine_->GetInputManager()->DPadUpPressed()) {
            currentPauseOption_ = PauseOption::ReturnToGame;
        } else if (IScene::PressedVK('S') || engine_->GetInputManager()->DPadDownPressed()) {
            currentPauseOption_ = PauseOption::ReturnToTitle;
        }

        // 決定キーが押されたら、決定演出に移行
        if (IScene::PressedVK(VK_SPACE) || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
            pauseMenuState_ = PauseMenuState::Confirming;
            confirmationTimer_ = 0.0f; // 決定演出タイマーをリセット
            se_select_->Play();
        }

        // 選択項目の明滅処理
        blinkTimer_ += 1.0f / 60.0f; // 60FPSを想定
        float alpha = 0.6f + 0.4f * std::sin(blinkTimer_ * BLINK_SPEED);

        // 全てのテキストを一旦不透明にリセット
        pauseReturnToGameText_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        pauseReturnToTitleText_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

        // 選択中の項目だけアルファ値を変更
        if (currentPauseOption_ == PauseOption::ReturnToGame) {
            pauseReturnToGameText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
        } else {
            pauseReturnToTitleText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
        }
    }
    // 決定演出中の処理
    else if (pauseMenuState_ == PauseMenuState::Confirming) {
        confirmationTimer_ += 1.0f / 60.0f;
        float alpha = std::fmod(confirmationTimer_ * CONFIRM_BLINK_SPEED, 1.0f) > 0.5f ? 1.0f : 0.0f;

        if (currentPauseOption_ == PauseOption::ReturnToGame) {
            pauseReturnToGameText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
        } else {
            pauseReturnToTitleText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
        }

        // 演出時間が終了したら、実際の処理を実行
        if (confirmationTimer_ >= CONFIRMATION_DURATION) {
            if (currentPauseOption_ == PauseOption::ReturnToGame) {
                engine_->GetSceneManager()->TogglePause(); // ポーズ解除
            } else {
                engine_->GetSceneManager()->Request("Title"); // タイトルへ
            }
            // 状態をリセット
            pauseMenuState_ = PauseMenuState::Selecting;
        }
    }

    // 各スプライトの更新
    pauseTitleText_->Update();
    pauseReturnToGameText_->Update();
    pauseReturnToTitleText_->Update();
}

void GameScene::PauseDraw()
{
    // ポーズ画面の描画
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    // 半透明の背景
    if (pauseSprite_) {
        pauseSprite_->Draw();
    }

    // メニュー項目の描画
    if (pauseTitleText_) {
        pauseTitleText_->Draw();
    }
    if (pauseReturnToGameText_) {
        pauseReturnToGameText_->Draw();
    }
    if (pauseReturnToTitleText_) {
        pauseReturnToTitleText_->Draw();
    }
}

void GameScene::InitializeSystem() {
    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->UpdateMatrix();

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());
    debugMode_ = false;

    // --- ライトの初期化 ---
    auto pointLight = std::make_unique<PointLight>();
    pointLight->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pointLight->position = { 0.0f, 5.0f, 0.0f };
    pointLight->intensity = 1.0f;
    pointLight->radius = 10.0f;
    pointLight->decay = 1.0f;
    pointLight->isActive = 1;
    pointLights_.push_back(std::move(pointLight));

    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLight_->direction = { 0.5f, -0.7f, 1.0f };
    directionalLight_->intensity = 1.0f;
}

void GameScene::InitializeGameObjects() {
    // マップチップフィールド
    mapChipField_ = std::make_unique<MapChipField>();
    std::string mapFileName = "resources/stage" + std::to_string(StageDataManager::selectedStageIndex + 1) + ".csv";
    mapChipField_->LoadMapChipCsv(mapFileName);

    // 天球
    skydome_ = std::make_unique<Skydome>();
    skydome_->Initialize(camera_.get());

    // ブロック
    blocks_ = std::make_unique<Region>();
    blocks_->Initialize(camera_.get(), "block.obj");
    GenerateBlocks();

    // 自キャラ
    player_ = std::make_shared<Player>();
    modelplayer_ = std::make_unique<ObjClass>();
    modelplayer_->Initialize(camera_.get(), "player.obj");
    modelplayerAttack_ = std::make_unique<ObjClass>();
    modelplayerAttack_->Initialize(camera_.get(), "attack.obj");
    modelplayerAttack_->SetColor(Vector4{ 85.9f / 255.0f, 89.8f / 255.0f, 52.9f / 255.0f, 1.0f });
    player_->SetAttackEffectModel(modelplayerAttack_.get());
    Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 18);
    player_->Initialize(modelplayer_.get(), camera_.get(), engine_->GetInputManager(), playerPosition);
    player_->SetMapChipField(mapChipField_.get());

    // 敵キャラ
    GenerateEnemies();

    // カメラコントローラー
    cameraController_ = std::make_unique<CameraController>();
    cameraController_->Initialize();
    cameraController_->SetTarget(player_.get());
    cameraController_->Reset();
}

void GameScene::InitializeUI() {
    // HP
    text_HP_ = std::make_unique<Sprite>();
    text_HP_->Initialize(camera_.get(), "resources/texture/gameText_HP.png");
    text_HP_->SetPosition(5.0f, 5.0f);

    // HPBar
    hpBar_out_ = std::make_unique<Sprite>();
    hpBar_out_->Initialize(camera_.get(), "resources/texture/hpBar_out.png");
    hpBar_out_->SetAnchor(0.0f, 0.0f);
    hpBar_out_->SetPosition(90.0f, 5.0f);

    hpBar_in_ = std::make_unique<Sprite>();
    hpBar_in_->Initialize(camera_.get(), "resources/texture/hpBar_in.png");
    hpBar_in_->SetAnchor(0.0f, 0.0f);
    hpBar_in_->SetPosition(91.0f, 6.0f);
    hpBar_in_->SetColor(Vector4{ 67.0f / 255.0f, 201.0f / 255.0f, 79.0f / 255.0f, 1.0f });
    hpBarOriginalWidth_ = hpBar_in_->GetSize().x;

    // フェード
    fade_ = std::make_unique<Fade>();
    fade_->Initialize(camera_.get());
    fade_->FadeIn(1.0f, { 0.0f, 0.0f, 0.0f, 1.0f });

    // ポーズ画面
    pauseSprite_ = std::make_unique<Sprite>();
    pauseSprite_->Initialize(camera_.get(), "resources/whiteTexture.png");
    pauseSprite_->SetPosition(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);
    pauseSprite_->SetSize(static_cast<float>(engine_->GetClientWidth()), static_cast<float>(engine_->GetClientHeight()));
    pauseSprite_->SetAnchor(0.5f, 0.5f);
    pauseSprite_->SetColor({ 0.1f, 0.1f, 0.1f, 0.5f });

    pauseTitleText_ = std::make_unique<Sprite>();
    pauseTitleText_->Initialize(camera_.get(), "resources/texture/pause_pause.png");
    pauseTitleText_->SetPosition(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);
    pauseTitleText_->SetAnchor(0.5f, 0.5f);

    pauseReturnToGameText_ = std::make_unique<Sprite>();
    pauseReturnToGameText_->Initialize(camera_.get(), "resources/texture/pause_backGame.png");
    pauseReturnToGameText_->SetPosition(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);
    pauseReturnToGameText_->SetAnchor(0.5f, 0.5f);

    pauseReturnToTitleText_ = std::make_unique<Sprite>();
    pauseReturnToTitleText_->Initialize(camera_.get(), "resources/texture/pause_backTitle.png");
    pauseReturnToTitleText_->SetPosition(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);
    pauseReturnToTitleText_->SetAnchor(0.5f, 0.5f);

    // カウントダウン
    text_1_ = std::make_unique<Sprite>();
    text_1_->Initialize(camera_.get(), "resources/texture/text_1.png");
    text_1_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);

    text_2_ = std::make_unique<Sprite>();
    text_2_->Initialize(camera_.get(), "resources/texture/text_2.png");
    text_2_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);

    text_3_ = std::make_unique<Sprite>();
    text_3_->Initialize(camera_.get(), "resources/texture/text_3.png");
    text_3_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);

    countdownText_killEnemy_ = std::make_unique<Sprite>();
    countdownText_killEnemy_->Initialize(camera_.get(), "resources/texture/text_killEnemy.png");
    countdownText_killEnemy_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f - 150.0f);

    // 操作方法
    manual_ = std::make_unique<Sprite>();
    manual_->Initialize(camera_.get(), "resources/texture/manual.png");

    // SE
    se_select_ = std::make_unique<Se>();
    se_select_->Initialize("resources/se/se_select.mp3");
}

void GameScene::GenerateEnemies() {
    // ステージ番号に応じて敵の生成パターンを切り替える
    switch (StageDataManager::selectedStageIndex) {
    case 0: // ステージ1
    {
        // NormalEnemyをマップチップ(10, 18)に配置
        auto enemy1 = std::make_unique<NormalEnemy>(this, camera_.get());
        Vector3 pos1 = mapChipField_->GetMapChipPositionByIndex(10, 18);
        enemy1->Initialize(pos1);
        enemy1->SetMapChipField(mapChipField_.get()); // マップチップフィールドをセット
        enemies_.push_back(std::move(enemy1));

        // さらに別のNormalEnemyをマップチップ(15, 18)に配置
        auto enemy2 = std::make_unique<NormalEnemy>(this, camera_.get());
        Vector3 pos2 = mapChipField_->GetMapChipPositionByIndex(15, 18);
        enemy2->Initialize(pos2);
        enemy2->SetMapChipField(mapChipField_.get()); // マップチップフィールドをセット
        enemies_.push_back(std::move(enemy2));
        break;
    }
    case 1: // ステージ2
    {
        // ShieldEnemyをマップチップ(8, 18)に配置
        auto enemy1 = std::make_unique<ShieldEnemy>(this, camera_.get());
        Vector3 pos1 = mapChipField_->GetMapChipPositionByIndex(8, 18);
        enemy1->Initialize(pos1);
        enemy1->SetMapChipField(mapChipField_.get()); // マップチップフィールドをセット
        enemies_.push_back(std::move(enemy1));

        // NormalEnemyをマップチップ(18, 18)に配置
        auto enemy2 = std::make_unique<ShieldEnemy>(this, camera_.get());
        Vector3 pos2 = mapChipField_->GetMapChipPositionByIndex(18, 18);
        enemy2->Initialize(pos2);
        enemy2->SetMapChipField(mapChipField_.get()); // マップチップフィールドをセット
        enemies_.push_back(std::move(enemy2));
        break;
    }
    default:
        // デフォルトの敵配置など
        break;
    }
}

void GameScene::CheckAllCollisions() {
    // --- プレイヤーの攻撃 vs 敵 ---
    if (player_->IsAttacking()) {
        const AABB attackAABB = player_->GetAttackAABB();
        for (const auto& enemy : enemies_) {
            if (enemy->IsDead()) {
                continue;
            }
            const AABB enemyAABB = enemy->GetAABB();
            if (Collision::IsAABBCollision(attackAABB, enemyAABB)) {
                // 敵側の衝突処理(プレイヤーのポインタを渡す)
                enemy->OnCollision(player_.get());
            }
        }
    }

    // --- プレイヤー本体 vs 敵 ---
    const AABB playerAABB = player_->GetAABB();
    for (const auto& enemy : enemies_) {
        if (enemy->IsDead()) {
            continue;
        }
        const AABB enemyAABB = enemy->GetAABB();
        if (Collision::IsAABBCollision(playerAABB, enemyAABB)) {
            // プレイヤー側の衝突処理
            player_->OnCollision(enemy.get());
        }
    }
}


void GameScene::GenerateBlocks() {

    // 要素数
    uint32_t numBlockVirtical = mapChipField_->GetNumBlockVirtical();
    uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

    // 要素数を変更する
    // 列数を指定(縦方向のブロック数)
    worldtransformBlocks_.resize(numBlockVirtical);
    for (uint32_t i = 0; i < numBlockVirtical; ++i) {
        // 1列の要素数を設定(横方向のブロック数)
        worldtransformBlocks_[i].resize(numBlockHorizontal);
    }

    // ブロックの生成
    // ブロックの生成(Blocks にインスタンスを積む)
    for (uint32_t i = 0; i < numBlockVirtical; ++i) {
        for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
            if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
                std::unique_ptr<Transform> worldTransform = std::make_unique<Transform>();
                worldtransformBlocks_[i][j] = worldTransform.get();
                worldtransformBlocks_[i][j]->translate = mapChipField_->GetMapChipPositionByIndex(j, i);
                // Blocksにもインスタンスとして追加
                if (blocks_) { blocks_->AddInstance(*worldtransformBlocks_[i][j]); }
            }
        }
    }
}

void GameScene::UpdateFadeIn()
{
    // フェードインが完了したらカウントダウンへ
    if (fade_->IsDone()) {
        phase_ = Phase::Countdown;
        countdownTimer_ = 3.0f; // カウントダウン開始
    }
}

void GameScene::UpdateCountdown()
{
    // カウントダウンタイマーを減らす
    countdownTimer_ -= 1.0f / 60.0f; // 60FPS想定

    // テキストの更新
    countdownText_killEnemy_->Update();

    // 各秒の開始時にアニメーションを適用
    float timeWithinSecond = std::fmod(countdownTimer_, 1.0f);
    if (timeWithinSecond > 0.98f) { // 1秒の始まりに近いタイミングで一度だけ実行
        float scale = 2.0f;
        float alpha = 0.0f;
        if (countdownTimer_ > 2.0f) {
            text_3_->GetD3D12Resource()->transform_.scale = { text_3_->GetSize().x * scale, text_3_->GetSize().y * scale, 1.0f };
            text_3_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
        } else if (countdownTimer_ > 1.0f) {
            text_2_->GetD3D12Resource()->transform_.scale = { text_2_->GetSize().x * scale, text_2_->GetSize().y * scale, 1.0f };
            text_2_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
        } else if (countdownTimer_ > 0.0f) {
            text_1_->GetD3D12Resource()->transform_.scale = { text_1_->GetSize().x * scale, text_1_->GetSize().y * scale, 1.0f };
            text_1_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
        }
    }

    // 1秒かけてスケールとアルファを変化させる
    float t = 1.0f - timeWithinSecond; // 0.0 -> 1.0
    float easedT = EaseOutQuint(t);

    float currentScale = Lerp(2.0f, 1.0f, easedT);
    float currentAlpha = Lerp(0.0f, 1.0f, easedT);

    // タイマーに応じて表示する数字を更新
    if (countdownTimer_ > 2.0f) {
        text_3_->GetD3D12Resource()->transform_.scale = { text_3_->GetSize().x * currentScale, text_3_->GetSize().y * currentScale, 1.0f };
        text_3_->SetColor({ 1.0f, 1.0f, 1.0f, currentAlpha });
        text_3_->Update();
    } else if (countdownTimer_ > 1.0f) {
        text_2_->GetD3D12Resource()->transform_.scale = { text_2_->GetSize().x * currentScale, text_2_->GetSize().y * currentScale, 1.0f };
        text_2_->SetColor({ 1.0f, 1.0f, 1.0f, currentAlpha });
        text_2_->Update();
    } else if (countdownTimer_ > 0.0f) {
        text_1_->GetD3D12Resource()->transform_.scale = { text_1_->GetSize().x * currentScale, text_1_->GetSize().y * currentScale, 1.0f };
        text_1_->SetColor({ 1.0f, 1.0f, 1.0f, currentAlpha });
        text_1_->Update();
    }

    // カウントダウンが終了したらゲームプレイへ
    if (countdownTimer_ <= 0.0f) {
        phase_ = Phase::Gameplay;
    }
}

void GameScene::UpdateGameplay()
{
    // 自キャラの更新
    player_->Update();

    // 敵キャラの更新
    for (const auto& enemy : enemies_) {
        enemy->Update();
    }

    // 衝突判定
    CheckAllCollisions();

    // 死亡した敵をリストから削除
    enemies_.erase(
        std::remove_if(
            enemies_.begin(),
            enemies_.end(),
            [](const std::unique_ptr<IEnemy>& enemy) { return enemy->IsDead(); }
        ),
        enemies_.end()
    );

    // UI/HUDの更新

    // HP
    text_HP_->Update();
    // HPBar
    // out
    hpBar_out_->Update();
    // in
    // HPの割合を計算
    float hpRatio = static_cast<float>(player_->GetHP()) / static_cast<float>(player_->GetMaxHP());
    hpRatio = std::clamp(hpRatio, 0.0f, 1.0f);

    // HPバーの幅を更新
    hpBar_in_->SetSize(hpBarOriginalWidth_ * hpRatio, hpBar_in_->GetSize().y);

    // HPに応じて色を緑から赤へ線形補間
    Vector4 green = { 67.0f / 255.0f, 201.0f / 255.0f, 79.0f / 255.0f, 1.0f };
    Vector4 red = { 1.0f, 0.0f, 0.0f, 1.0f };
    // HPが50%を切ったら赤に近づける
    float colorRatio = std::clamp((1.0f - hpRatio) * 2.0f, 0.0f, 1.0f);
    hpBar_in_->SetColor(Lerp(green, red, colorRatio));

    hpBar_in_->Update();

    // ゲームオーバー条件(プレイヤー死亡)
    if (player_->IsDead()) {
        GameResultManager::result = GameResultManager::Result::Lose;
        phase_ = Phase::FadeOut;
        fade_->FadeOut(1.0f, { 0.0f, 0.0f, 0.0f, 1.0f }); // 黒色で1秒間のフェードアウト
    }
    // ゲームクリア条件(例：敵が全滅)
    else if (enemies_.empty()) {
        GameResultManager::result = GameResultManager::Result::Win;
        phase_ = Phase::FadeOut;
        fade_->FadeOut(1.0f, { 1.0f, 1.0f, 1.0f, 1.0f }); // 白色で1秒間のフェードアウト
    }
}

void GameScene::UpdateFadeOut()
{
    // フェードアウトが完了したらリザルトシーンへ
    if (fade_->IsDone()) {
        engine_->GetSceneManager()->Request("Result");
    }
}