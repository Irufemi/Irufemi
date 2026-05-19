#include "GameApplication.h"

#include <memory>
#include <string>

#include "Engine/Irufemi.h"
#include "Framework/SceneManager.h"

// memoryでの未定義

#include "Framework/Component/ComponentFactory.h"
#include "components/RailPathComponent.h"
#include "components/RailShooterPlayerComponent.h"
#include "components/RailShooterEnemyComponent.h"
#include "components/CameraFollowPlayerComponent.h"
#include "components/DebrisComponent.h"
#include "components/DebrisManagerComponent.h"

// シーンのインクルード
#include "scene/title/TitleScene.h"
#include "scene/stageSelect/SelectScene.h"
#include "scene/inGame/GameScene.h"
#include "scene/Clear/ClearScene.h"
#include "scene/GameOver/GameOverScene.h"
#include "scene/Pause/PauseScene.h"
#if defined(_DEBUG) || defined(DEVELOPMENT)
#include "scene/debug/DebugScene.h"
#endif

namespace {
    // --- ゲーム固有の定数 ---
    const int32_t kClientWidth = 1280;
    const int32_t kClientHeight = 720;
    const std::wstring kTitle = L"Application_solo";
    const Vector4 kClearColor = { 0.1f, 0.25f, 0.5f, 1.0f };
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
#if defined(_DEBUG) || defined(DEVELOPMENT)
        sm.Register("Debug", [] { return std::make_unique<DebugScene>(); });
#endif

    }
}

GameApplication::GameApplication() = default;
GameApplication::~GameApplication() = default;

void GameApplication::Run() {
    // エンジンのインスタンスを生成
    auto engine = std::make_unique<IrufemiEngine>();

    // エンジンの初期化
    engine->Initialize(kTitle, kClientWidth, kClientHeight, kClearColor);

    // 独自コンポーネントの登録
    ComponentFactory::Register("RailPathComponent", []() { return std::make_shared<RailPathComponent>(); });
    ComponentFactory::Register("RailShooterPlayerComponent", []() { return std::make_shared<RailShooterPlayerComponent>(); });
    ComponentFactory::Register("RailShooterEnemyComponent", []() { return std::make_shared<RailShooterEnemyComponent>(); });
    ComponentFactory::Register("CameraFollowPlayerComponent", []() { return std::make_shared<CameraFollowPlayerComponent>(); });
    ComponentFactory::Register("DebrisComponent", []() { return std::make_shared<DebrisComponent>(); });
    ComponentFactory::Register("DebrisManagerComponent", []() { return std::make_shared<DebrisManagerComponent>(); });

    // シーンの登録
    engine->SetSceneRegistrar(RegisterScenes);

    // 初期シーンの設定
    engine->SetInitialSceneName(kInitialScene);

    // ゲームループの実行
    engine->Execute();
}
