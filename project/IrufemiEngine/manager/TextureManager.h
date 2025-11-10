#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>  
#include <d3d12.h>
#include <wrl.h>
#include "source/Texture.h" 

class DirectXCommon;

class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager() = default;

    // 初期化
    void Initialize(DirectXCommon* dxCommon);

    // フォルダからロード
    void LoadAllFromFolder(const std::string& folderPath);

    // ファイルパス/名前でSRVハンドルを取得（未ロードならロードしてキャッシュ）
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(const std::string& name) const;

    // テクスチャ名一覧
    std::vector<std::string> GetTextureNames() const;

    // 白テクスチャの作成（フォールバック）
    void CreateWhiteDummyTexture();

    // 互換API：SRVインデックスの取得/増加（既存コードと互換）
    uint32_t GetSRVIndex() const;
    void AddSRVIndex();

    // テクスチャサイズ取得
    bool GetTextureSize(const std::string& name, uint32_t& outWidth, uint32_t& outHeight) const;

    // 白テクスチャハンドル取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetWhiteTextureHandle() const { return whiteTextureHandle; }

private:
    // TextureManager 内で完結する SRV 割り当て（スレッド安全）
    static uint32_t AllocateSrvIndexSafe(DirectXCommon* dxCommon) noexcept;
    static std::mutex srvIndexMutex_; // 実体は cpp に定義

private:
    DirectXCommon* dxCommon_ = nullptr;

    // key: ファイルパス（または識別名）、value: Texture オブジェクト
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures_;

    // フォールバック白テクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource> whiteTextureResource;
    D3D12_GPU_DESCRIPTOR_HANDLE whiteTextureHandle{ 0 };

};