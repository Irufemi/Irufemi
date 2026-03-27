#include "TextureManager.h"

#include <filesystem>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <format>
#include <cstdint>

#include "Engine/Graphics/DirectX/DescriptorPool.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "DirectXTex/DirectXTex.h"
#include "DirectXTex/d3dx12.h"

static bool IsImageExtImpl(const std::string& extLower) {
    static const char* exts[] = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds" };
    for (auto* e : exts) {
        if (extLower == e) { return true; }
    }
    return false;
}

// Initialize: DirectXCommon を保存し、Texture にも渡す
void TextureManager::Initialize(DirectXCommon* dxCommon) {
    dxCommon_ = dxCommon;
    Texture::SetDirectXCommon(dxCommon_);
    // フォールバックを必ず作る
    CreateWhiteDummyTexture();
}

// 指定フォルダ配下を走査してロード(キーはフルパス文字列)
void TextureManager::LoadAllFromFolder(const std::string& folderPath) {
    namespace fs = std::filesystem;
    fs::path root(folderPath);
    if (!fs::exists(root)) { return; }

    for (auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) { continue; }
        auto p = entry.path();
        auto ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (!IsImageExtImpl(ext)) { continue; }

        const std::string key = p.generic_string();
        // 既にあるならスキップ
        if (textures_.find(key) != textures_.end()) { continue; }

        // 実際にロードしてキャッシュ
        auto tex = std::make_shared<Texture>();
        tex->Initialize(key);
        textures_.emplace(key, std::move(tex));
    }
}

// 取得(未ロードならロードしてキャッシュ)
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetTextureHandle(const std::string& name) const {
    // 既存キー検索
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        D3D12_GPU_DESCRIPTOR_HANDLE h = it->second->GetTextureSrvHandleGPU();
        if (h.ptr == 0) {
            // フォールバック
            if (whiteTextureHandle_.ptr != 0) return whiteTextureHandle_;
        }
        return h;
    }

    // キャッシュ更新
    auto tex = std::make_shared<Texture>();
    tex->Initialize(name);
    D3D12_GPU_DESCRIPTOR_HANDLE handle = tex->GetTextureSrvHandleGPU();
    if (handle.ptr == 0 && whiteTextureHandle_.ptr != 0) {
        handle = whiteTextureHandle_;
    }
    textures_.emplace(name, std::move(tex));
    return handle;
}

const DirectX::ScratchImage* TextureManager::GetScratchImage(const std::string& name) const
{
    // 既存キー検索
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        return it->second->GetScratchImage();
    }

    // キャッシュになければロード
    auto tex = std::make_shared<Texture>();
    tex->Initialize(name);
    textures_.emplace(name, tex);
    return tex->GetScratchImage();
}

std::vector<std::string> TextureManager::GetTextureNames() const {
    std::vector<std::string> keys;
    keys.reserve(textures_.size());
    for (auto& kv : textures_) keys.push_back(kv.first);
    return keys;
}

void TextureManager::CreateWhiteDummyTexture() {
    if (whiteTextureHandle_.ptr != 0) return;
    if (!dxCommon_) { OutputDebugStringA("CreateWhiteDummyTexture: dxCommon_ is null\n"); return; }

    // 2x2 白テクスチャ
    uint32_t whitePixels[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };

    auto tex = std::make_shared<Texture>();
    tex->InitializeFromMemory("white", whitePixels, 2, 2);
    
    whiteTextureHandle_ = tex->GetTextureSrvHandleGPU();
    textures_.emplace("white", tex);

    // ログ
    auto msg = std::format("CreateWhiteDummyTexture: created 'white' texture handle ptr={:#x}\n",
        static_cast<uintptr_t>(whiteTextureHandle_.ptr));
    OutputDebugStringA(msg.c_str());
}

bool TextureManager::GetTextureSize(const std::string& name, uint32_t& outWidth, uint32_t& outHeight) const {
    auto it = textures_.find(name);
    if (it == textures_.end()) { return false; }
    outWidth = it->second->GetWidth();
    outHeight = it->second->GetHeight();
    return true;
}