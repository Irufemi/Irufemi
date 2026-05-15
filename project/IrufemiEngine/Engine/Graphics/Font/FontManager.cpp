#include "FontManager.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Core/System/ThreadPool.h"
#include "Engine/Core/Utility/Log.h"

// --- 外部ライブラリ群 ---
#include <ft2build.h>
#include FT_FREETYPE_H

#undef min
#undef max
#include <msdfgen/msdfgen.h>
#include <msdfgen/ext/import-font.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>

#include <unordered_map>
#include <vector>
#include <mutex>
#include <algorithm>

// 内部実装の定義
struct FontManager::Impl {
    msdfgen::FreetypeHandle* ftLibrary = nullptr;
    std::unordered_map<std::string, msdfgen::FontHandle*> fonts;

    // キャッシュされた文字情報: FontID -> (文字コード -> GlyphInfo)
    std::unordered_map<std::string, std::unordered_map<char32_t, GlyphInfo>> glyphCache;
    std::mutex cacheMutex;

    // アトラス管理 (stb_rect_pack用)
    stbrp_context packContext;
    std::vector<stbrp_node> packNodes;
    std::vector<uint8_t> cpuAtlasData; // RGBA
    
    // DirectX12 用の動的テクスチャリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> atlasTexture;
    D3D12_GPU_DESCRIPTOR_HANDLE atlasSrv{};

    static const int ATLAS_WIDTH = 2048;
    static const int ATLAS_HEIGHT = 2048;
    static const int GLYPH_SIZE = 32; // ベースサイズ
    static const int PADDING = 2; // エッジパディング
    static constexpr double PX_RANGE = 2.0; // 距離場スプレッド
};

FontManager::FontManager() : impl_(std::make_unique<Impl>()) {}

FontManager::~FontManager() {
    Finalize();
}

void FontManager::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
    
    // FreeTypeの初期化
    impl_->ftLibrary = msdfgen::initializeFreetype();
    
    // CPUアトラスの初期化
    impl_->cpuAtlasData.resize(Impl::ATLAS_WIDTH * Impl::ATLAS_HEIGHT * 4, 0); // 0クリア(RGBA)
    
    // stb_rect_pack のコンテキスト初期化
    impl_->packNodes.resize(Impl::ATLAS_WIDTH);
    stbrp_init_target(&impl_->packContext, Impl::ATLAS_WIDTH, Impl::ATLAS_HEIGHT, impl_->packNodes.data(), static_cast<int>(impl_->packNodes.size()));

    // TODO: 2048x2048 の空のID3D12Resource (アトラステクスチャ) を生成しSRVを作成
    // DirectXCommon経由で生成を行う (後日TextureManager等と統合)
}

void FontManager::Finalize() {
    for (auto& pair : impl_->fonts) {
        if (pair.second) {
            msdfgen::destroyFont(pair.second);
        }
    }
    impl_->fonts.clear();

    if (impl_->ftLibrary) {
        msdfgen::deinitializeFreetype(impl_->ftLibrary);
        impl_->ftLibrary = nullptr;
    }
}

bool FontManager::LoadFont(const std::string& fontId, const std::string& ttfPath) {
    msdfgen::FontHandle* font = msdfgen::loadFont(impl_->ftLibrary, ttfPath.c_str());
    if (font) {
        std::lock_guard<std::mutex> lock(impl_->cacheMutex);
        impl_->fonts[fontId] = font;
        return true;
    }
    return false;
}

void FontManager::PrecacheText(const std::string& fontId, const std::wstring& text) {
    // 非同期化する場合は、ここで engine_->GetThreadPool()->Enqueue(...) でタスクを投げる
    // 今回はまず基本実装として同期的(またはThreadPool内で呼ばれる想定)に処理する

    std::lock_guard<std::mutex> lock(impl_->cacheMutex);
    auto it = impl_->fonts.find(fontId);
    if (it == impl_->fonts.end()) return;
    msdfgen::FontHandle* font = it->second;

    auto& fontCache = impl_->glyphCache[fontId];

    msdfgen::FontMetrics metrics;
    msdfgen::getFontMetrics(metrics, font, msdfgen::FONT_SCALING_NONE);

    for (wchar_t c : text) {
        char32_t char32 = static_cast<char32_t>(c);
        
        if (fontCache.find(char32) == fontCache.end()) {
            msdfgen::Shape shape;
            double advance = 0.0;
            
            // スペースなどの空文字対策
            if (char32 == U' ') {
                double spaceAdvance = 0.0, tabAdvance = 0.0;
                msdfgen::getFontWhitespaceWidth(spaceAdvance, tabAdvance, font, msdfgen::FONT_SCALING_NONE);
                GlyphInfo info{};
                info.character = char32;
                info.advanceX = static_cast<float>(spaceAdvance);
                fontCache[char32] = info;
                continue;
            }

            if (msdfgen::loadGlyph(shape, font, char32, msdfgen::FONT_SCALING_NONE, &advance)) {
                shape.normalize();
                msdfgen::edgeColoringSimple(shape, 3.0);

                msdfgen::Shape::Bounds bounds = shape.getBounds();
                // フォントの高さと幅を算出
                double width = bounds.r - bounds.l;
                double height = bounds.t - bounds.b;

                // スケールの決定 (GLYPH_SIZE に収まるように)
                double scale = static_cast<double>(Impl::GLYPH_SIZE) / (metrics.emSize > 0.0 ? metrics.emSize : 1.0);
                
                int texWidth = static_cast<int>(width * scale) + Impl::PADDING * 2;
                int texHeight = static_cast<int>(height * scale) + Impl::PADDING * 2;

                if (texWidth <= 0 || texHeight <= 0) continue;

                // オフセット計算 (テクスチャ中央に配置)
                msdfgen::Vector2 translate(-bounds.l + (Impl::PADDING / scale), -bounds.b + (Impl::PADDING / scale));

                // MSDF用ビットマップ
                msdfgen::Bitmap<float, 3> msdf(texWidth, texHeight);
                msdfgen::generateMSDF(msdf, shape, msdfgen::Projection(msdfgen::Vector2(scale), translate), Impl::PX_RANGE, msdfgen::MSDFGeneratorConfig());

                // stbrp_pack_rects でパッキング
                stbrp_rect rect{};
                rect.w = texWidth;
                rect.h = texHeight;
                if (stbrp_pack_rects(&impl_->packContext, &rect, 1) == 1) {
                    // CPUアトラスにコピー (上下反転に対応するためY座標を反転しながらコピー)
                    for (int y = 0; y < texHeight; ++y) {
                        for (int x = 0; x < texWidth; ++x) {
                            const float* pixel = msdf(x, texHeight - 1 - y); // Y反転
                            int destX = rect.x + x;
                            int destY = rect.y + y;
                            int destIndex = (destY * Impl::ATLAS_WIDTH + destX) * 4;
                            
                            impl_->cpuAtlasData[destIndex + 0] = static_cast<uint8_t>(std::clamp(pixel[0] * 255.f, 0.f, 255.f)); // R
                            impl_->cpuAtlasData[destIndex + 1] = static_cast<uint8_t>(std::clamp(pixel[1] * 255.f, 0.f, 255.f)); // G
                            impl_->cpuAtlasData[destIndex + 2] = static_cast<uint8_t>(std::clamp(pixel[2] * 255.f, 0.f, 255.f)); // B
                            impl_->cpuAtlasData[destIndex + 3] = 255; // A
                        }
                    }

                    // GlyphInfo作成
                    GlyphInfo info{};
                    info.character = char32;
                    info.uvTopLeft = Vector2(static_cast<float>(rect.x) / Impl::ATLAS_WIDTH, static_cast<float>(rect.y) / Impl::ATLAS_HEIGHT);
                    info.uvBottomRight = Vector2(static_cast<float>(rect.x + rect.w) / Impl::ATLAS_WIDTH, static_cast<float>(rect.y + rect.h) / Impl::ATLAS_HEIGHT);
                    info.width = static_cast<float>(rect.w);
                    info.height = static_cast<float>(rect.h);
                    info.offsetX = static_cast<float>(bounds.l * scale) - Impl::PADDING;
                    info.offsetY = static_cast<float>((metrics.ascenderY - bounds.t) * scale) - Impl::PADDING; // ベースライン基準のオフセット
                    info.advanceX = static_cast<float>(advance * scale);

                    fontCache[char32] = info;

                    // TODO: VRAM(GPU)への転送リクエストを発行
                    // engine_->GetDirectXCommon()->UpdateTextureRegion(impl_->atlasTexture.Get(), rect.x, rect.y, rect.w, rect.h, msdfデータの先頭等);
                }
            }
        }
    }
}

const GlyphInfo* FontManager::GetGlyph(const std::string& fontId, char32_t character) {
    {
        std::lock_guard<std::mutex> lock(impl_->cacheMutex);
        auto& fontCache = impl_->glyphCache[fontId];
        auto it = fontCache.find(character);
        
        if (it != fontCache.end()) {
            return &it->second;
        }
    }

    // まだ生成されていなければその場で生成予約を入れる（現在は即時生成）
    std::wstring singleChar;
    singleChar += static_cast<wchar_t>(character);
    PrecacheText(fontId, singleChar);
    
    std::lock_guard<std::mutex> lock(impl_->cacheMutex);
    return &impl_->glyphCache[fontId][character];
}

D3D12_GPU_DESCRIPTOR_HANDLE FontManager::GetAtlasSRV() const {
    return impl_->atlasSrv;
}
