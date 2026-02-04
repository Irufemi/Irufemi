#include "GameApplication.h"

#include <memory>
#include <string>
#include "math/Vector4.h"

#include "engine/IrufemiEngine.h"
#include "scene/SceneManager.h"

// memoryでの未定義
#include "camera/DebugCamera.h"
#include "2D/Sprite.h"
#include "contents/UI/NumberText.h"

// シーンのインクルード
#include "scene/title/TitleScene.h"
#include "scene/stageSelect/SelectScene.h"
#include "scene/inGame/GameScene.h"
#include "scene/result/ResultScene.h"
#if defined(_DEBUG) || defined(DEVELOPMENT)
#include "scene/debug/DebugScene.h"
#endif

namespace {
    // --- ゲーム固有の定数 ---
    const int32_t kClientWidth = 1280;
    const int32_t kClientHeight = 720;
    const std::wstring kTitle = L"2326_血管壊回";
    const Vector4 kClearColor = { 0.7f, 0.3f, 0.3f, 1.0f };
    const char kInitialScene[]
#if defined(_DEBUG) || defined(DEVELOPMENT)
        = "Title";
#else
        = "Title";
#endif

    // --- シーン登録処理 ---
    void RegisterScenes(SceneManager& sm) {
        sm.Register("Title", [] { return std::make_unique<TitleScene>(); });
        sm.Register("Select", [] { return std::make_unique<SelectScene>(); });
        sm.Register("InGame", [] { return std::make_unique<GameScene>(); });
        sm.Register("Result", [] { return std::make_unique<ResultScene>(); });
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