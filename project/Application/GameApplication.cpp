#include "GameApplication.h"

#include <memory>
#include <string>

#include "Engine/Irufemi.h"
#include "Framework/SceneManager.h"

// memoryでの未定義

// シーンのインクルード
#include "scene/title/TitleScene.h"
#include "scene/stageSelect/SelectScene.h"
#include "scene/inGame/GameScene.h"
#include "scene/Clear/ClearScene.h"
#include "scene/GameOver/GameOverScene.h"
#if defined(_DEBUG) || defined(DEVELOPMENT)
#include "scene/debug/DebugScene.h"
#endif

namespace {
    // --- ゲーム固有の定数 ---
    const int32_t kClientWidth = 1280;
    const int32_t kClientHeight = 720;
    const std::wstring kTitle = L"3122_七転び八転び";
    const Vector4 kClearColor = { 0.1f, 0.25f, 0.5f, 1.0f };
    const char kInitialScene[]
#if defined(_DEBUG) || defined(DEVELOPMENT)
        = "InGame";
#else
        = "Title";
#endif

    // --- シーン登録処理 ---
    void RegisterScenes(SceneManager& sm) {
        sm.Register("Title", [] { return std::make_unique<TitleScene>(); });
        //sm.Register("Select", [] { return std::make_unique<SelectScene>(); });
        sm.Register("InGame", [] { return std::make_unique<GameScene>(); });
        sm.Register("Clear", [] { return std::make_unique<ClearScene>(); });
        sm.Register("GameOver", [] { return std::make_unique<GameOverScene>(); });
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

    // シーンの登録
    engine->SetSceneRegistrar(RegisterScenes);

    // 初期シーンの設定
    engine->SetInitialSceneName(kInitialScene);

    // ゲームループの実行
    engine->Execute();
}
