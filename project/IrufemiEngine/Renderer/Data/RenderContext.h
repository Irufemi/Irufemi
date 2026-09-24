#pragma once
#include <d3d12.h>
#include <cstdint>

class IrufemiEngine;
class DirectXCommon;
class DrawManager;
class TextureManager;
class FontManager;
class PSOManager;

namespace Irufemi {

/**
 * @struct RenderContext
 * @brief 描画フレームおよび各レンダーパスの実行に必要なコンテキスト情報を集約する構造体
 */
struct RenderContext {
    IrufemiEngine* engine = nullptr;
    DirectXCommon* dxCommon = nullptr;
    DrawManager* drawManager = nullptr;
    TextureManager* textureManager = nullptr;
    FontManager* fontManager = nullptr;
    PSOManager* psoManager = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;
    uint32_t frameIndex = 0;
};

} // namespace Irufemi
