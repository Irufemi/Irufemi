#include "GameApplication.h"

#include <memory>
#include <string>

#include "Engine/Irufemi.h"
#include "Framework/SceneManager.h"

// memoryでの未定義

#include "Framework/Component/ComponentFactory.h"
#include "components/RailShooterPlayerComponent.h"
#include "components/RailShooterEnemyComponent.h"
#include "components/DebrisComponent.h"
#include "components/DebrisManagerComponent.h"
#include "components/GravityPlayerComponent.h"
#include "components/DebugEnemySpawnerComponent.h"
#include "components/BossComponent.h"
#include "components/SceneTransitionButtonComponent.h"
#include "components/EffectManagerComponent.h"

// エンジン機能
#include "Engine/Graphics/DirectX/ShaderManager.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"

// UI
#include "UI/LoadingScreen.h"

// シーンのインクルード
#include "scene/title/TitleScene.h"
#include "scene/stageSelect/SelectScene.h"
#include "scene/inGame/GameScene.h"
#include "scene/Clear/ClearScene.h"
#include "scene/GameOver/GameOverScene.h"
#include "scene/Pause/PauseScene.h"
#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
#include "scene/debug/DebugScene.h"
#endif

#ifdef EditorMode
#include "EditorManager.h"
#endif

namespace {
    // --- ゲーム固有の定数 ---
    const int32_t kClientWidth = 1280;
    const int32_t kClientHeight = 720;
    const std::wstring kTitle = L"Application_solo";
    const Vector4 kClearColor = { 0.5f, 0.5f, 0.5f, 1.0f };
    const char kInitialScene[]
#if defined(_DEBUG) || defined(DEVELOPMENT)
        = "Debug";
#else
        = "Title";
#endif

    // --- シーン登録処理 ---
    void RegisterScenes(SceneManager& sm) {
        sm.Register("Title", [] { return std::make_unique<TitleScene>(); });
        sm.Register("Select", [] { return std::make_unique<SelectScene>(); });
        sm.Register("InGame", [] { return std::make_unique<GameScene>(); });
        sm.Register("Clear", [] { return std::make_unique<ClearScene>(); });
        sm.Register("GameOver", [] { return std::make_unique<GameOverScene>(); });
        sm.Register("Pause", [] { return std::make_unique<PauseScene>(); });
#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
        sm.Register("Debug", [] { return std::make_unique<DebugScene>(); });
#endif

    }
}

GameApplication::GameApplication() = default;
GameApplication::~GameApplication() = default;

void GameApplication::Run() {
    // エンジンのインスタンスを生成
    auto engine = std::make_unique<IrufemiEngine>();

#ifdef EditorMode
    // エディタマネージャを拡張として事前登録（Initialize時に初期化される）
    engine->AddExtension(std::make_shared<EditorManager>());
#endif

    // エンジンの初期化
    engine->Initialize(kTitle, kClientWidth, kClientHeight, kClearColor);

    // アプリ固有のシェーダー登録
    {
        ShaderCompileOptions options;
#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
        options.isDebug = true;
#endif
        auto shaderManager = engine->GetDirectXCommon()->GetShaderManager();
        auto psoManager = engine->GetPSOManager();
        
        auto vs3d = shaderManager->GetOrCompile(L"resources/shaders/Object3D.VS.hlsl", options);
        auto psEnergyCore = shaderManager->GetOrCompile(L"resources/shaders/EnergyCore.PS.hlsl", options);
        psoManager->RegisterShader("EnergyCore", { { vs3d, psEnergyCore } });

        // 追加: EnergyBeam と LightningCrawl の登録
        auto psEnergyBeam = shaderManager->GetOrCompile(L"resources/shaders/EnergyBeam.PS.hlsl", options);
        psoManager->RegisterShader("EnergyBeam", { { vs3d, psEnergyBeam } });

        auto psLightningCrawl = shaderManager->GetOrCompile(L"resources/shaders/LightningCrawl.PS.hlsl", options);
        psoManager->RegisterShader("LightningCrawl", { { vs3d, psLightningCrawl } });
    }

    // 独自コンポーネントの登録
    ComponentFactory::Register("RailShooterPlayerComponent", "Game", []() { return std::make_shared<RailShooterPlayerComponent>(); });
    ComponentFactory::Register("RailShooterEnemyComponent", "Game", []() { return std::make_shared<RailShooterEnemyComponent>(); });
    ComponentFactory::Register("DebrisComponent", "Game", []() { return std::make_shared<DebrisComponent>(); });
    ComponentFactory::Register("DebrisManagerComponent", "Game", []() { return std::make_shared<DebrisManagerComponent>(); });
    ComponentFactory::Register("GravityPlayerComponent", "Game", []() { return std::make_shared<GravityPlayerComponent>(); });
    ComponentFactory::Register("DebugEnemySpawnerComponent", "Game", []() { return std::make_shared<DebugEnemySpawnerComponent>(); });
    ComponentFactory::Register("BossComponent", "Game", []() { return std::make_shared<BossComponent>(); });
    ComponentFactory::Register("SceneTransitionButtonComponent", "Game", []() { return std::make_shared<SceneTransitionButtonComponent>(); });
    ComponentFactory::Register("EffectManagerComponent", "Game", []() { return std::make_shared<EffectManagerComponent>(); });
    // UIの登録
    auto loadingScreen = std::make_shared<LoadingScreen>();
    loadingScreen->Initialize(engine.get());
    engine->SetLoadingScreen(loadingScreen);

    // シーンの登録
    engine->SetSceneRegistrar(RegisterScenes);

    // 初期シーンの設定
    engine->SetInitialSceneName(kInitialScene);

    // ゲームループの実行
    engine->Execute();
}
