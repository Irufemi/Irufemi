#include "GameApplication.h"

#include <memory>
#include <string>

#include "Engine/Irufemi.h"
#include "Framework/SceneManager.h"

// memoryでの未定義

#include "Framework/Component/ComponentFactory.h"
#include "components/RailShooterPlayerComponent.h"
#include "components/SplineFollowerComponent.h"
#include "components/RailRelativeFollowerComponent.h"
#include "components/RailShooterEnemyComponent.h"
#include "components/DebrisComponent.h"
#include "components/TargetableComponent.h"
#include "components/DebrisManagerComponent.h"
#include "components/WaveManagerComponent.h"
#include "components/GravityPlayerComponent.h"
#include "components/PlayerTargetingComponent.h"
#include "components/DebugEnemySpawnerComponent.h"
#include "components/Boss/BossComponent.h"
#include "components/SceneTransitionButtonComponent.h"
#include "components/EffectManagerComponent.h"
#include "components/EnvironmentManagerComponent.h"
#include "UI/ReticleUIComponent.h"
#include "UI/LockonMarkerUIComponent.h"
#include "components/DroneManagerComponent.h"
#include "components/BossBulletManagerComponent.h"
#include "components/GameLoopManagerComponent.h"

// エンジン機能
#include "Engine/Graphics/DirectX/ShaderManager.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"

// UI
#include "UI/LoadingScreen.h"

// シーンのインクルード
#include "scene/title/TitleScene.h"
#include "scene/stageSelect/SelectScene.h"
#include "scene/inGame/GameScene.h"
#include "scene/Pause/PauseScene.h"
#include "scene/TL1/TL1Scene.h"

#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
#include "Framework/DebugScene.h"
#endif
#include "scene/TL1/TL1Scene.h"

#ifdef EditorMode
#include "EditorManager.h"
#include "Editor/Core/ComponentEditorRegistry.h"
#include "components/editor/WaveManagerComponentEditor.h"
#include "components/editor/GravityPlayerComponentEditor.h"
#include "components/editor/BossComponentEditor.h"
#endif

namespace {
    // --- ゲーム固有の定数 ---
    const int32_t kClientWidth = 1280;
    const int32_t kClientHeight = 720;
    const std::wstring kTitle = L"Application_solo";
    const Irufemi::Vector4 kClearColor = { 0.08f, 0.03f, 0.02f, 1.0f }; // 退廃的な荒野（ダーク・ラスト）
    const char kInitialScene[]
#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
        = "InGame";
#else
        = "Title";
#endif

    // --- シーン登録処理 ---
    void RegisterScenes(SceneManager& sm) {
        sm.Register("Title", [] { return std::make_unique<TitleScene>(); });
        sm.Register("Select", [] { return std::make_unique<SelectScene>(); });
        sm.Register("InGame", [] { return std::make_unique<GameScene>(); });
        sm.Register("Pause", [] { return std::make_unique<PauseScene>(); });
        sm.Register("TL1", [] { return std::make_unique<TL1Scene>(); });

#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
        sm.Register("Debug", [] { return std::make_unique<DebugScene>(); });
#endif
        sm.Register("TL1", [] { return std::make_unique<TL1Scene>(); });
    }
}

GameApplication::GameApplication() = default;
GameApplication::~GameApplication() = default;

void GameApplication::Run() {
    // エンジンのインスタンスを生成
    auto engine = std::make_unique<IrufemiEngine>();

#ifdef EditorMode
    // エディタマネージャを拡張として事前登録（Initialize時に初期化される）
    auto editorManager = std::make_shared<EditorManager>();
    engine->AddExtension(editorManager);
#endif

    // エンジンの初期化
    engine->Initialize(kTitle, kClientWidth, kClientHeight, kClearColor);

#ifdef EditorMode
    // エンジン初期化後にエディタへ登録（OnInitialize内でレジストリが生成されるため）
    if (auto registry = editorManager->GetComponentEditorRegistry()) {
        registry->RegisterEditor<WaveManagerComponent, WaveManagerComponentEditor>();
        registry->RegisterEditor<GravityPlayerComponent, GravityPlayerComponentEditor>();
        registry->RegisterEditor<BossComponent, BossComponentEditor>();
    }
#endif

    // アプリ固有のシェーダー登録
    {
        ShaderCompileOptions options;
#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
        options.isDebug = true;
#endif
        auto shaderManager = engine->GetDirectXCommon()->GetShaderManager();
        auto psoManager = engine->GetPSOManager();
        
        auto vs3d = shaderManager->GetOrCompile(L"Object3d.VS.hlsl", options);
        auto psEnergyCore = shaderManager->GetOrCompile(L"EnergyCore.PS.hlsl", options);
        psoManager->RegisterShader("EnergyCore", { { vs3d, psEnergyCore } });

        // 追加: EnergyBeam と LightningCrawl の登録
        auto psEnergyBeam = shaderManager->GetOrCompile(L"EnergyBeam.PS.hlsl", options);
        psoManager->RegisterShader("EnergyBeam", { { vs3d, psEnergyBeam } });

        auto psLightningCrawl = shaderManager->GetOrCompile(L"LightningCrawl.PS.hlsl", options);
        psoManager->RegisterShader("LightningCrawl", { { vs3d, psLightningCrawl } });

        // LockonMarker用シェーダー (SpriteBatch.VS.hlsl を使う)
        auto vsSpriteBatch = shaderManager->GetOrCompile(L"SpriteBatch.VS.hlsl", options);
        auto psLuminanceAlpha = shaderManager->GetOrCompile(L"LuminanceAlpha2D.PS.hlsl", options);
        psoManager->RegisterShader("LuminanceAlpha2D", { { vsSpriteBatch, psLuminanceAlpha } });
    }

    // 独自コンポーネントの登録
    ComponentFactory::Register("RailShooterPlayerComponent", "Game", []() { return std::make_shared<RailShooterPlayerComponent>(); });
    ComponentFactory::Register("SplineFollowerComponent", "Game", []() { return std::make_shared<SplineFollowerComponent>(); });
    ComponentFactory::Register("RailRelativeFollowerComponent", "Game", []() { return std::make_shared<RailRelativeFollowerComponent>(); });
    ComponentFactory::Register("RailShooterEnemyComponent", "Game", []() { return std::make_shared<RailShooterEnemyComponent>(); });
    ComponentFactory::Register("DebrisComponent", "Game", []() { return std::make_shared<DebrisComponent>(); });
    ComponentFactory::Register("TargetableComponent", "Game", []() { return std::make_shared<TargetableComponent>(); });
    ComponentFactory::Register("DebrisManagerComponent", "Game", []() { return std::make_shared<DebrisManagerComponent>(); });
    ComponentFactory::Register("WaveManagerComponent", "Game", []() { return std::make_shared<WaveManagerComponent>(); });
    ComponentFactory::Register("EnvironmentManagerComponent", "Game", []() { return std::make_shared<EnvironmentManagerComponent>(); });
    ComponentFactory::Register("GravityPlayerComponent", "Game", []() { return std::make_shared<GravityPlayerComponent>(); });
    ComponentFactory::Register("PlayerTargetingComponent", "Game", []() { return std::make_shared<PlayerTargetingComponent>(); });
    ComponentFactory::Register("DebugEnemySpawnerComponent", "Game", []() { return std::make_shared<DebugEnemySpawnerComponent>(); });
    ComponentFactory::Register("BossComponent", "Game", []() { return std::make_shared<BossComponent>(); });
    ComponentFactory::Register("SceneTransitionButtonComponent", "Game", []() { return std::make_shared<SceneTransitionButtonComponent>(); });
    ComponentFactory::Register("EffectManagerComponent", "Game", []() { return std::make_shared<EffectManagerComponent>(); });
    ComponentFactory::Register("ReticleUIComponent", "UI", []() { return std::make_shared<ReticleUIComponent>(); });
    ComponentFactory::Register("LockonMarkerUIComponent", "UI", []() { return std::make_shared<LockonMarkerUIComponent>(); });
    ComponentFactory::Register("DroneManagerComponent", "Game", []() { return std::make_shared<DroneManagerComponent>(); });
    ComponentFactory::Register("BossBulletManagerComponent", "Game", []() { return std::make_shared<BossBulletManagerComponent>(); });

    ComponentFactory::Register("GameLoopManagerComponent", "Game", []() { return std::make_shared<GameLoopManagerComponent>(); });
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
