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

    // --- ライトの初期化 ---
    pointLight_ = std::make_unique <PointLight>();
    pointLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pointLight_->position = { 0.0f, 5.0f, 0.0f };
    pointLight_->intensity = 1.0f;
    pointLight_->radius = 10.0f;
    pointLight_->decay = 1.0f;

    spotLight_ = std::make_unique <SpotLight>();
    spotLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    spotLight_->position = { 2.0f, 1.25f, 0.0f };
    spotLight_->distance = 7.0f;
    spotLight_->direction = Math::Normalize(Vector3{ -1.0f,-1.0f,0.0f });
    spotLight_->intensity = 0.0f; // 初期状態ではOFF
    spotLight_->decay = 2.0f;
    spotLight_->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);

    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f,1.0f,1.0f,1.0f };
    directionalLight_->direction = { 0.5f,-0.7f,1.0f };
    directionalLight_->intensity = 1.0f;

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
    if (ImGui::CollapsingHeader("PointLight")) {
        ImGui::ColorEdit4("PointLightColor", &pointLight_->color.x);
        ImGui::DragFloat3("PointLightPosition", &pointLight_->position.x, 0.01f);
        ImGui::DragFloat("PointLightIntensity", &pointLight_->intensity, 0.01f, 0.0f);
        ImGui::DragFloat("PointLightRadius", &pointLight_->radius, 0.01f, 0.0f);
        ImGui::DragFloat("PointLightDecay", &pointLight_->decay, 0.01f, 0.0f);
    }
    // spotLight 
    if (ImGui::CollapsingHeader("SpotLight")) {
        ImGui::ColorEdit4("SpotLightColor", &spotLight_->color.x);
        ImGui::DragFloat3("SpotLightPosition", &spotLight_->position.x, 0.01f);
        ImGui::DragFloat("SpotLightIntensity", &spotLight_->intensity, 0.01f, 0.0f);
        ImGui::DragFloat3("SpotLightDirection", &spotLight_->direction.x, 0.01f);
        spotLight_->direction = Math::Normalize(spotLight_->direction);
        ImGui::DragFloat("SpotLightDistance", &spotLight_->distance, 0.01f, 0.0f);
        ImGui::DragFloat("SpotLightDecay", &spotLight_->decay, 0.01f, 0.0f);
        ImGui::DragFloat("SpotLightCosAngle", &spotLight_->cosAngle, 0.01f, 0.0f, 1.0f);
    }
    // directionalLight
    if (ImGui::CollapsingHeader("DirectionalLight")) {
        ImGui::ColorEdit4("DirectionalLightColor", &directionalLight_->color.x);
        ImGui::DragFloat3("DirectionalLightDirection", &directionalLight_->direction.x, 0.01f);
        directionalLight_->direction = Math::Normalize(directionalLight_->direction);
        ImGui::DragFloat("DirectionalLightIntensity", &directionalLight_->intensity, 0.01f, 0.0f);
    }

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

    // --- カメラの更新 ---
    // 現在アクティブなカメラへのポインタ
    Camera* currentCamera = debugMode ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera"); // デバッグカメラも通常カメラもUpdateを呼ぶ

    // デバッグモードでない場合のみ、カメラコントローラーを適用
    if (!debugMode) {
        cameraController_->Update(*camera_.get());
    }

    // 天球の更新
    skydome_->Update();

    if (engine_->GetInputManager()->IsKeyPressed('P') || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
        engine_->GetSceneManager()->Request("Title");
    }

    // --- フレーム共通データのセット ---
    CameraForGPU cameraForGpu;
    cameraForGpu.view = currentCamera->GetViewMatrix();
    cameraForGpu.projection = currentCamera->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = currentCamera->GetTranslate();
    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, *pointLight_, *spotLight_);
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