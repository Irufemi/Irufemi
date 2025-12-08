#include "GameScene.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"


#include "camera/Camera.h"
#include "camera/DebugCamera.h"

// デストラクタ
GameScene::~GameScene() {

}

// 初期化
void GameScene::Initialize(IrufemiEngine* engine) {

    // 参照したものをコピー
    // エンジン
    this->engine_ = engine;

    camera_ = std::make_unique <Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());

    debugCamera_ = std::make_unique <DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());
    debugMode = false;

    pointLight_ = std::make_unique <PointLightClass>();
    pointLight_->Initialize();
    pointLight_->SetPos(Vector3{ 0.0f,-5.0f,0.0f });

    engine_->GetDrawManager()->SetPointLightClass(pointLight_.get());

    spotLight_ = std::make_unique <SpotLightClass>();
    spotLight_->Initialize();
    spotLight_->SetIntensity(0.0f);

    engine_->GetDrawManager()->SetSpotLightClass(spotLight_.get());

    /// マップチップフィールド
    // マップチップフィールドの生成
    mapChipField_ = std::make_unique<MapChipField>();
    // マップチップフィールドのファイル読み込み
    mapChipField_->LoadMapChipCsv("resources/blocks.csv");

    /// 天球
    // 天球の生成
    skydome_ = std::make_unique<Skydome>();
    // 天球の初期化
    skydome_->Initialize(camera_.get());

    /// ブロック
    // ブロックの初期化

    /// ブロック
    // ブロックの初期化（Blocksでまとめて管理）
    blocks_ = std::make_unique<Region>();
    blocks_->Initialize(camera_.get(), "block.obj");
    GenerateBlocks();

    /// 自キャラ
    // 自キャラの生成
    player_ = std::make_shared<Player>();
    // 3Dモデルデータの生成
    modelplayer_ = std::make_unique<ObjClass>();
    modelplayer_->Initialize(camera_.get(), "player.obj");
    modelplayerAttack_ = std::make_unique<ObjClass>();
    modelplayerAttack_->Initialize(camera_.get(), "player_attackEffect.obj");
    player_->SetAttackModel(modelplayerAttack_.get());
    // 座標をマップチップ番号で指定
    Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 18);
    // 自キャラの初期化
    player_->Initialize(modelplayer_.get(), camera_.get(), engine->GetInputManager(), playerPosition);
    // マップチップデータのセット
    player_->SetMapChipField(mapChipField_.get());

    /// カメラコントローラー
    // カメラコントローラーの生成
    cameraController_ = std::make_unique<CameraController>();
    // カメラコントローラーの初期化
    cameraController_->Initialize();
    // 追従対象をセット
    cameraController_->Settarget(player_.get());
    // リセット(瞬間合わせ)
    cameraController_->Reset();
}

// 更新
void GameScene::Update() {

#if defined USE_IMGUI

    ImGui::Begin("GameScene");
    // pointLight 
    pointLight_->Debug();
    // spotLight 
    spotLight_->Debug();

    ImGui::End();

    ImGui::Begin("Texture");
    if (ImGui::Button("allLoadActivate")) {
        engine_->GetTextureManager()->LoadAllFromFolder("resources/");
    }
    ImGui::Checkbox("debugMode", &debugMode);
    ImGui::End();

#endif // _DEBUG

    // 自キャラの更新
    player_->Update();

    // カメラの更新
    if (debugMode) {
        debugCamera_->Update();
        camera_->SetViewMatrix(debugCamera_->GetCamera().GetViewMatrix());
        camera_->SetPerspectiveFovMatrix(debugCamera_->GetCamera().GetPerspectiveFovMatrix());
    } else {
        camera_->Update("Camera");
        // カメラコントローラーの更新
        cameraController_->Update(*camera_.get());
    }
    
    // 天球の更新
    skydome_->Update();

    if (engine_->GetInputManager()->IsKeyPressed('P') || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
        engine_->GetSceneManager()->Request("Title");
    }

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
    
    // Region
    engine_->ApplyRegionPSO();

    // ブロック
    blocks_->Draw();

    // Particle
    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplyParticlePSO();

    // Sprite

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

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
    // ブロックの生成（Blocks にインスタンスを積む）
    for (uint32_t i = 0; i < numBlockVirtical; ++i) {
        for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
            if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
                Transform* worldTransform = new Transform();
                worldtransformBlocks_[i][j] = worldTransform;
                worldtransformBlocks_[i][j]->translate = mapChipField_->GetMapChipPositionByIndex(j, i);
                // Blocksにもインスタンスとして追加
                if (blocks_) { blocks_->AddInstance(*worldtransformBlocks_[i][j]); }
            }
        }
    }
}