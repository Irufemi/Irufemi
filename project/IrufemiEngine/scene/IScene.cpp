#include "IScene.h"
#include "../engine/IrufemiEngine.h"
#include "../engine/Input/InputManager.h"
#include <cstring>

unsigned char IScene::keys_[256]{};
unsigned char IScene::preKeys_[256]{};
unsigned char IScene::dikKeys_[256]{};
unsigned char IScene::dikPreKeys_[256]{};

void IScene::SyncInput(IrufemiEngine* engine) {
    // 直前を保存
    std::memcpy(preKeys_, keys_, 256);
    std::memcpy(dikPreKeys_, dikKeys_, 256);

    auto* in = engine->GetInputManager();

    // VKスナップショット
    for (int k = 0; k < 256; ++k) {
        keys_[k] = in->IsKeyDown(static_cast<uint8_t>(k)) ? 0x80u : 0x00u;
    }
    // DIKスナップショット
    for (int k = 0; k < 256; ++k) {
        dikKeys_[k] = in->IsKeyDownDIK(static_cast<uint8_t>(k)) ? 0x80u : 0x00u;
    }
}