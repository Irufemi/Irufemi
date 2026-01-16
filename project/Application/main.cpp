#include <Windows.h>
#include <memory>
#include "GameApplication.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    // ゲームアプリケーションの生成と実行
    auto game = std::make_unique<GameApplication>();
    game->Run();

    return 0;
}