#include "TextureManager.h"

#include <filesystem>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <format>
#include <cstdint>

#include "engine/DescriptorAllocator.h"
#include "engine/directX/DirectXCommon.h"
#include "externals/DirectXTex/DirectXTex.h"
#include <externals/DirectXTex/d3dx12.h>

// 静的メンバ実体
std::mutex TextureManager::srvIndexMutex_;

static bool IsImageExtImpl(const std::string& extLower) {
    static const char* exts[] = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds" };
    for (auto* e : exts) {
        if (extLower == e) { return true; }
    }
    return false;
}

// TextureManager::AllocateSrvIndexSafe (クラス内に閉じる実装)
uint32_t TextureManager::AllocateSrvIndexSafe(DirectXCommon* dxCommon) noexcept {
    std::lock_guard<std::mutex> lk(srvIndexMutex_);

    if (!dxCommon) {
        OutputDebugStringA("TextureManager::AllocateSrvIndexSafe: dxCommon is null\n");
        return UINT32_MAX;
    }

    auto heap = dxCommon->GetSrvDescriptorHeap();
    if (!heap) {
        OutputDebugStringA("TextureManager::AllocateSrvIndexSafe: srvDescriptorHeap is null\n");
        return UINT32_MAX;
    }

    D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();

    // 既存実装と互換性を維持するため Texture::index_ を参照する
    uint32_t index = Texture::GetStaticSRVIndex();
    if (index >= desc.NumDescriptors) {
        auto msg = std::format("TextureManager::AllocateSrvIndexSafe: index {} >= heap.NumDescriptors {}\n", index, desc.NumDescriptors);
        OutputDebugStringA(msg.c_str());
        return UINT32_MAX;
    }

    // 既存実装互換：静的インデックスを進める。ここで保護される。
    Texture::AddStaticSRVIndex();

    return index;
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

    if (!dxCommon_->GetSrvDescriptorHeap()) {
        OutputDebugStringA("CreateWhiteDummyTexture: srvDescriptorHeap is null, deferring white texture creation\n");
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
    // ここでハンドルを外側で宣言して、以降でも参照できるようにする
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};

    {
        // まずアロケータ優先
        uint32_t index = DescriptorAllocator::kInvalid;

        if (Texture::GetDescriptorAllocator()) {
            auto alloc = Texture::GetDescriptorAllocator();
            index = alloc->Allocate();
            if (index != DescriptorAllocator::kInvalid) {
                cpuHandle = alloc->GetCPUHandle(index);
                gpuHandle = alloc->GetGPUHandle(index);
            } else {
                OutputDebugStringA("CreateWhiteDummyTexture: allocator exhausted, using heap start fallback\n");
                cpuHandle = dxCommon_->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
                gpuHandle = dxCommon_->GetSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
            }
        } else {
            cpuHandle = dxCommon_->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
            gpuHandle = dxCommon_->GetSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        dxCommon_->GetDevice()->CreateShaderResourceView(whiteTextureResource.Get(), &srvDesc, cpuHandle);
        whiteTextureHandle = gpuHandle;
    }

    // ログ（↑で確定した cpuHandle/gpuHandle を使用）
    {
        auto msg = std::format("CreateWhiteDummyTexture: created SRV cpu.ptr={:#x} gpu.ptr={:#x}\n",
            static_cast<uintptr_t>(cpuHandle.ptr),
            static_cast<uintptr_t>(gpuHandle.ptr));
        OutputDebugStringA(msg.c_str());
    }
}

// 既存API互換：Texture 側の静的SRVインデックスに委譲
uint32_t TextureManager::GetSRVIndex() const {
    return Texture::GetStaticSRVIndex();
}
void TextureManager::AddSRVIndex() {
    Texture::AddStaticSRVIndex();
}

bool TextureManager::GetTextureSize(const std::string& name, uint32_t& outWidth, uint32_t& outHeight) const {
    auto it = textures_.find(name);
    if (it == textures_.end()) { return false; }
    outWidth = it->second->GetWidth();
    outHeight = it->second->GetHeight();
    return true;
}