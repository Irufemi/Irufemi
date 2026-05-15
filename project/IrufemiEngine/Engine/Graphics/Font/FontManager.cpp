#include "FontManager.h"
#include "Engine/IrufemiEngine.h"

// --- 外部ライブラリ群 (実際の導入後にコメントアウトを解除して使用) ---
// #include <ft2build.h>
// #include FT_FREETYPE_H
// #include <msdfgen.h>
// #include <msdfgen-ext.h>
// #define STB_RECT_PACK_IMPLEMENTATION
// #include <stb_rect_pack.h>

#include <unordered_map>
#include <vector>

// 内部実装の定義
struct FontManager::Impl {
    // FT_Library ftLibrary;
    // std::unordered_map<std::string, FT_Face> fonts;

    // キャッシュされた文字情報: FontID -> (文字コード -> GlyphInfo)
    std::unordered_map<std::string, std::unordered_map<char32_t, GlyphInfo>> glyphCache;

    // アトラス管理 (stb_rect_pack用)
    // stbrp_context packContext;
    // std::vector<stbrp_node> packNodes;
    
    // DirectX12 用の動的テクスチャリソース
    // Microsoft::WRL::ComPtr<ID3D12Resource> atlasTexture;
    // D3D12_GPU_DESCRIPTOR_HANDLE atlasSrv;

    static const int ATLAS_WIDTH = 2048;
    static const int ATLAS_HEIGHT = 2048;
};

FontManager::FontManager() : impl_(std::make_unique<Impl>()) {}

FontManager::~FontManager() {
    Finalize();
}

void FontManager::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
    
    // TODO: FreeTypeの初期化 (FT_Init_FreeType)
    
    // TODO: 2048x2048 の空のID3D12Resource (アトラステクスチャ) を生成しSRVを作成
    // ※エンジン既存の TextureManager または DirectXCommon を活用する
    
    // TODO: stb_rect_pack のコンテキスト初期化 (stbrp_init_target)
}

void FontManager::Finalize() {
    // TODO: FreeTypeの解放 (FT_Done_FreeType) など
}

bool FontManager::LoadFont(const std::string& fontId, const std::string& ttfPath) {
    // TODO: FT_New_Face を用いてTTFをロードし、impl_->fonts に登録する
    return true;
}

void FontManager::PrecacheText(const std::string& fontId, const std::wstring& text) {
    for (wchar_t c : text) {
        char32_t char32 = static_cast<char32_t>(c);
        
        // キャッシュに存在するか確認
        auto& fontCache = impl_->glyphCache[fontId];
        if (fontCache.find(char32) == fontCache.end()) {
            
            // --- キャッシュになければ動的生成 ---
            // 1. FreeType でグリフのアウトラインを取得
            // 2. msdfgen にアウトラインを渡し、MSDFビットマップを生成
            // 3. stbrp_pack_rects でアトラス上の空き領域 (X, Y) を確保
            // 4. engine_->GetDirectXCommon()->UploadTextureData() 等を用いて、
            //    Uploadヒープ経由で DirectX12テクスチャの (X, Y) にMSDFビットマップを部分転送する
            // 5. GlyphInfo を作成してキャッシュに登録
            // ※将来的にはこれを ThreadPool を用いて非同期で実行する
            
            GlyphInfo info{};
            info.character = char32;
            // ... (UVやサイズの計算)
            fontCache[char32] = info;
        }
    }
}

const GlyphInfo* FontManager::GetGlyph(const std::string& fontId, char32_t character) {
    auto& fontCache = impl_->glyphCache[fontId];
    auto it = fontCache.find(character);
    
    if (it != fontCache.end()) {
        return &it->second;
    }

    // まだ生成されていなければその場で生成予約を入れる（現在は即時生成）
    std::wstring singleChar;
    singleChar += static_cast<wchar_t>(character);
    PrecacheText(fontId, singleChar);
    
    return &fontCache[character];
}

D3D12_GPU_DESCRIPTOR_HANDLE FontManager::GetAtlasSRV() const {
    // return impl_->atlasSrv;
    return D3D12_GPU_DESCRIPTOR_HANDLE{};
}
