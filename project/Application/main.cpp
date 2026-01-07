#include <Windows.h>
#include <cstdint>
#include <memory>
#include <string>
#include "math/Vector4.h"

#include "engine/IrufemiEngine.h"

#include "scene/title/TitleScene.h"
#include "scene/inGame/GameScene.h"
#include "scene/result/ResultScene.h"
#include "scene/debug/DebugScene.h"
#include "scene/stageSelect/SelectScene.h"

//クライアント領域のサイズ
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

// タイトル
const std::wstring kTitle = L"LE2B_12_スエヒロ_コウイチ_アンナイト";

// ウィンドウの色
//const Vector4 clearColor = { 0.1f, 0.25f, 0.5f, 1.0f };
const Vector4 clearColor = { 0.7f, 0.7f, 0.7f, 1.0f };

//windowsアプリでのエントリーポint32_tイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    std::unique_ptr<IrufemiEngine> engine = std::make_unique<IrufemiEngine>();

    // 例1: 背景色を黒に
    engine->Initialize(kTitle, kClientWidth, kClientHeight, Vector4{ clearColor });

    // 例2: 実行中に色を変えたい場合
    // engine->SetClearColor(0.2f, 0.2f, 0.25f, 1.0f);

    // アプリ側で登録
    engine->SetSceneRegistrar([](SceneManager& sm) {
        sm.Register("Title", [] { return std::make_unique<TitleScene>();  });
        sm.Register("Select", [] { return std::make_unique<SelectScene>();  });
        sm.Register("InGame", [] { return std::make_unique<GameScene>();   });
        sm.Register("Result", [] { return std::make_unique<ResultScene>(); });
        sm.Register("Debug", [] { return std::make_unique<DebugScene>(); });
        }
    );

    engine->SetInitialSceneName("Title");

    engine->Execute();

    return 0;

}