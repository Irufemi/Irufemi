#include "GameApplication.h"

#include <memory>
#include <string>
#include "math/Vector4.h"

#include "engine/IrufemiEngine.h"
#include "scene/SceneManager.h"

// ゲームシーンのインクルード
#include "scene/title/TitleScene.h"
#include "scene/stageSelect/SelectScene.h"
#include "scene/inGame/GameScene.h"
#include "scene/result/ResultScene.h"
#include "scene/debug/DebugScene.h"

namespace {
    // --- ゲーム固有の定数 ---
    const int32_t kClientWidth = 1280;
    const int32_t kClientHeight = 720;
    const std::wstring kTitle = L"2326_血管壊回";
    const Vector4 kClearColor = { 0.5f, 0.5f, 0.5f, 1.0f };
    const char kInitialScene[] = "InGame";

    // --- シーン登録処理 ---
    void RegisterScenes(SceneManager& sm) {
        sm.Register("Title", [] { return std::make_unique<TitleScene>(); });
        sm.Register("Select", [] { return std::make_unique<SelectScene>(); });
        sm.Register("InGame", [] { return std::make_unique<GameScene>(); });
        sm.Register("Result", [] { return std::make_unique<ResultScene>(); });
        sm.Register("Debug", [] { return std::make_unique<DebugScene>(); });
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