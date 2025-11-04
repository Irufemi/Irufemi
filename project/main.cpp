#include <Windows.h>
#include <cstdint>
#include <memory>

#include "engine/IrufemiEngine.h"

//クライアント領域のサイズ
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

// タイトル
const std::wstring kTitle = L"";

//windowsアプリでのエントリーポint32_tイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    std::unique_ptr<IrufemiEngine> engine = std::make_unique<IrufemiEngine>();
    engine->Initialize(kTitle, kClientWidth, kClientHeight);

    engine->Execute();

    return 0;

}