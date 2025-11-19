#include "TextureManager.h"

#include <filesystem>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <format>
#include <cstdint>

#include "engine/directX/DescriptorPool.h"
#include "engine/directX/DirectXCommon.h"
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

// 指定フォルダ配下を走査してロード（キーはフルパス文字列）
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

        const std::string key = p.string();
        // 既にあるならスキップ
        if (textures_.find(key) != textures_.end()) { continue; }

        // 実際にロードしてキャッシュ
        auto tex = std::make_shared<Texture>();
        tex->Initialize(key);
        textures_.emplace(key, std::move(tex));
    }
}

// 取得（未ロードならロードしてキャッシュ）
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetTextureHandle(const std::string& name) const {
    // 既存キー検索
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        D3D12_GPU_DESCRIPTOR_HANDLE h = it->second->GetTextureSrvHandleGPU();
        if (h.ptr == 0) {
            // フォールバック
            if (whiteTextureHandle.ptr != 0) return whiteTextureHandle;
        }
        return h;
    }

    // キャッシュ更新
    auto* self = const_cast<TextureManager*>(this);
    auto tex = std::make_shared<Texture>();
    tex->Initialize(name);
    D3D12_GPU_DESCRIPTOR_HANDLE handle = tex->GetTextureSrvHandleGPU();
    if (handle.ptr == 0 && whiteTextureHandle.ptr != 0) {
        handle = whiteTextureHandle;
    }
    self->textures_.emplace(name, std::move(tex));
    return handle;
}

std::vector<std::string> TextureManager::GetTextureNames() const {
    std::vector<std::string> keys;
    keys.reserve(textures_.size());
    for (auto& kv : textures_) keys.push_back(kv.first);
    return keys;
}

void TextureManager::CreateWhiteDummyTexture() {
    if (whiteTextureHandle.ptr != 0) return;
    if (!dxCommon_) { OutputDebugStringA("CreateWhiteDummyTexture: dxCommon_ is null\n"); return; }

    DescriptorPool* srvPool = dxCommon_->GetSrvPool();
    if (!srvPool) {
        OutputDebugStringA("CreateWhiteDummyTexture: srvPool is null, deferring white texture creation\n");
        return;
    }

    // 2x2 白テクスチャ
    uint32_t whitePixels[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };

    D3D12_RESOURCE_DESC texDesc{};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = 2;
    texDesc.Height = 2;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProp{};
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProp, D3D12_HEAP_FLAG_NONE,
        &texDesc, D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&whiteTextureResource)
    );
    if (FAILED(hr)) { OutputDebugStringA("CreateWhiteDummyTexture: CreateCommittedResource(texture) failed\n"); return; }

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(whitePixels));
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadResource;
    hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadResource)
    );
    if (FAILED(hr)) { OutputDebugStringA("CreateWhiteDummyTexture: CreateCommittedResource(upload) failed\n"); return; }

    uint8_t* mapped = nullptr;
    hr = uploadResource->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
    if (FAILED(hr) || !mapped) { OutputDebugStringA("CreateWhiteDummyTexture: Map(upload) failed\n"); return; }
    std::copy_n(reinterpret_cast<const uint8_t*>(whitePixels),
                sizeof(whitePixels),
                mapped);
    uploadResource->Unmap(0, nullptr);

    D3D12_SUBRESOURCE_DATA subresource{};
    subresource.pData = whitePixels;
    subresource.RowPitch = texDesc.Width * 4;
    subresource.SlicePitch = subresource.RowPitch * texDesc.Height;

    UpdateSubresources(dxCommon_->GetCommandList(), whiteTextureResource.Get(), uploadResource.Get(), 0, 0, 1, &subresource);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = whiteTextureResource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);

    hr = dxCommon_->GetCommandList()->Close();
    if (FAILED(hr)) { OutputDebugStringA("CreateWhiteDummyTexture: Close command list failed\n"); return; }
    ID3D12CommandList* lists[] = { dxCommon_->GetCommandList() };
    dxCommon_->GetCommandQueue()->ExecuteCommandLists(1, lists);

    uint64_t& fenceValue = dxCommon_->GetFenceValue();
    ++fenceValue;
    dxCommon_->GetCommandQueue()->Signal(dxCommon_->GetFence(), fenceValue);
    if (dxCommon_->GetFence()->GetCompletedValue() < fenceValue) {
        dxCommon_->GetFence()->SetEventOnCompletion(fenceValue, dxCommon_->GetFenceEvent());
        WaitForSingleObject(dxCommon_->GetFenceEvent(), INFINITE);
    }

    hr = dxCommon_->GetCommandAllocator()->Reset();
    if (FAILED(hr)) { OutputDebugStringA("CreateWhiteDummyTexture: Reset CommandAllocator failed\n"); return; }
    hr = dxCommon_->GetCommandList()->Reset(dxCommon_->GetCommandAllocator(), nullptr);
    if (FAILED(hr)) { OutputDebugStringA("CreateWhiteDummyTexture: Reset CommandList failed\n"); return; }

    // SRV 割当て
    const uint32_t index = srvPool->Allocate();
    assert(index != DescriptorPool::kInvalid && "DescriptorPool exhausted. Cannot create white texture.");

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvPool->GetCPUHandle(index);
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvPool->GetGPUHandle(index);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    dxCommon_->GetDevice()->CreateShaderResourceView(whiteTextureResource.Get(), &srvDesc, cpuHandle);
    whiteTextureHandle = gpuHandle;

    // ログ
    auto msg = std::format("CreateWhiteDummyTexture: created SRV cpu.ptr={:#x} gpu.ptr={:#x} at index {}\n",
        static_cast<uintptr_t>(cpuHandle.ptr),
        static_cast<uintptr_t>(gpuHandle.ptr),
        index);
    OutputDebugStringA(msg.c_str());
}

bool TextureManager::GetTextureSize(const std::string& name, uint32_t& outWidth, uint32_t& outHeight) const {
    auto it = textures_.find(name);
    if (it == textures_.end()) { return false; }
    outWidth = it->second->GetWidth();
    outHeight = it->second->GetHeight();
    return true;
}