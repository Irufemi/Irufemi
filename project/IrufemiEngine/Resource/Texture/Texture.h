#pragma once

#include <d3d12.h>
#include <string>
#include <wrl.h>
#include <cstdint>
#include "../../../externals/DirectXTex/DirectXTex.h"

class DirectXCommon;
class DescriptorPool;

/**
 * @class Texture
 * @brief 個別のテクスチャリソースを管理するクラス
 * @details DirectX 12 のリソース（ID3D12Resource）と SRV ハンドルを保持し、データのロードと初期化を行います。
 */
class Texture {
public:
    /** @name 静的メンバ設定 */
    ///@{
    static void SetDirectXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
    static void SetDescriptorPool(DescriptorPool* pool) { s_srvPool_ = pool; }
    static DescriptorPool* GetDescriptorPool() { return s_srvPool_; }
    ///@}

    /**
     * @brief コンストラクタ
     */
    Texture() ;

    /**
     * @brief デストラクタ
     */
    ~Texture();

    /**
     * @brief ファイルからテクスチャを初期化
     * @param[in] filePath 画像ファイルのパス
     */
    void Initialize(const std::string& filePath);

    /**
     * @brief メモリ上のピクセルデータからテクスチャを初期化
     * @param[in] name 識別名
     * @param[in] pixels ピクセルデータ（RGBA8想定）
     * @param[in] width 横幅
     * @param[in] height 縦幅
     */
    void InitializeFromMemory(const std::string& name, const uint32_t* pixels, uint32_t width, uint32_t height);

    /**
     * @brief GPU側のSRVハンドルを取得
     */
    const D3D12_GPU_DESCRIPTOR_HANDLE& GetTextureSrvHandleGPU()const { return textureSrvHandleGPU_; }

    /**
     * @brief ScratchImage（CPU側の画像データ）を取得
     */
    const DirectX::ScratchImage* GetScratchImage() const { return &mipImages_; }

    /** @name サイズ取得 */
    ///@{
    uint32_t GetWidth()  const { return width_; }
    uint32_t GetHeight() const { return height_; }
    ///@}

    /** @name 下位互換用（段階移行用） */
    ///@{
    static uint32_t GetStaticSRVIndex() { return index_; }
    static void AddStaticSRVIndex() { index_++; }
    ///@}

protected:
    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};
    std::string filePath_;
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource_ = nullptr;
    DirectX::ScratchImage mipImages_;

    static uint32_t index_; // 互換用
    uint32_t srvIndex_ = UINT32_MAX; // allocator で確保した index

    static DirectXCommon* dxCommon_;
    static DescriptorPool* s_srvPool_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

