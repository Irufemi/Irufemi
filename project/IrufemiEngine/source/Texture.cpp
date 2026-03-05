#include "Texture.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include "engine/directX/DirectXCommon.h"
#include "engine/directX/DescriptorPool.h"

DirectXCommon* Texture::dxCommon_ = nullptr;
uint32_t Texture::index_ = 0;
DescriptorPool* Texture::s_srvPool_ = nullptr;

Texture::Texture() = default;

Texture::~Texture() {
    if (s_srvPool_ && srvIndex_ != UINT32_MAX && dxCommon_) {
        // GPU が参照し終わるまで遅延解放
        s_srvPool_->FreeAfterFence(srvIndex_, dxCommon_->GetFenceValue());
        srvIndex_ = UINT32_MAX;
    }
}

void Texture::Initialize(const std::string& filePath) {
    this->filePath_ = filePath;

    DirectX::ScratchImage mipImages = dxCommon_->LoadTexture(filePath_);
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
    width_ = static_cast<uint32_t>(metadata.width);
    height_ = static_cast<uint32_t>(metadata.height);

    textureResource_ = dxCommon_->CreateTextureResource(metadata);
    intermediateResource_ = dxCommon_->UploadTextureData(textureResource_.Get(), mipImages);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (metadata.IsCubemap()) {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = UINT_MAX;
        srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    }
    else {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);
    }


    // 1) allocator 優先
    uint32_t indexForSrv = UINT32_MAX;
    if (s_srvPool_) {
        indexForSrv = s_srvPool_->Allocate();
        if (indexForSrv != DescriptorPool::kInvalid) {
            srvIndex_ = indexForSrv;
            textureSrvHandleCPU_ = s_srvPool_->GetCPUHandle(indexForSrv);
            textureSrvHandleGPU_ = s_srvPool_->GetGPUHandle(indexForSrv);
        } else {
            indexForSrv = UINT32_MAX; // fallback
        }
    }
    // 2) fallback: 既存の静的カウンタ
    if (indexForSrv == UINT32_MAX) {
        index_ += 1;
        indexForSrv = index_;
        if (s_srvPool_) {
            textureSrvHandleCPU_ = s_srvPool_->GetCPUHandle(indexForSrv);
            textureSrvHandleGPU_ = s_srvPool_->GetGPUHandle(indexForSrv);
        } else {
            // s_srvPool_ が nullptr の場合は安全なデフォルト値を設定
            textureSrvHandleCPU_ = D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
            textureSrvHandleGPU_ = D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
        }
    }

    dxCommon_->GetDevice()->CreateShaderResourceView(textureResource_.Get(), &srvDesc, textureSrvHandleCPU_);
}
