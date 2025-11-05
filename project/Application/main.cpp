#include <Windows.h>
#include <cstdint>
#include <memory>
#include <string>

#include "engine/IrufemiEngine.h"

#include "scene/title/TitleScene.h"
#include "scene/inGame/GameScene.h"
#include "scene/result/ResultScene.h"

//クライアント領域のサイズ
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

// タイトル
const std::wstring kTitle = L"";

//windowsアプリでのエントリーポint32_tイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    std::unique_ptr<IrufemiEngine> engine = std::make_unique<IrufemiEngine>();
    engine->Initialize(kTitle, kClientWidth, kClientHeight);

    // アプリ側で登録
    engine->SetSceneRegistrar([](SceneManager& sm) {
        sm.Register("Title", [] { return std::make_unique<TitleScene>();  });
        sm.Register("InGame", [] { return std::make_unique<GameScene>();   });
        sm.Register("Result", [] { return std::make_unique<ResultScene>(); });
        }
    );

    engine->SetInitialSceneName("Title");

    engine->Execute();

    return 0;

}