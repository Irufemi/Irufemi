#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>  
#include <d3d12.h>
#include <wrl.h>
#include "Resource/Texture/Texture.h" 

// 前方宣言
namespace DirectX {
    class ScratchImage;
}

class DirectXCommon;

class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager() = default;

    // 初期化
    void Initialize(DirectXCommon* dxCommon);

    // フォルダからロード
    void LoadAllFromFolder(const std::string& folderPath);

    // ファイルパス/名前でSRVハンドルを取得(未ロードならロードしてキャッシュ)
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(const std::string& name) const;

    // CPUでアクセス可能な画像データを取得
    const DirectX::ScratchImage* GetScratchImage(const std::string& name) const;

    // テクスチャ名一覧
    std::vector<std::string> GetTextureNames() const;

    // 白テクスチャの作成(フォールバック)
    void CreateWhiteDummyTexture();

    // テクスチャサイズ取得
    bool GetTextureSize(const std::string& name, uint32_t& outWidth, uint32_t& outHeight) const;

    // 白テクスチャハンドル取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetWhiteTextureHandle() const { return whiteTextureHandle; }

private:
    DirectXCommon* dxCommon_ = nullptr;

    // key: ファイルパス(または識別名)、value: Texture オブジェクト
    mutable std::unordered_map<std::string, std::shared_ptr<Texture>> textures_;

    // フォールバック白テクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource> whiteTextureResource;
    D3D12_GPU_DESCRIPTOR_HANDLE whiteTextureHandle{ 0 };

};