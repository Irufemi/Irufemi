#pragma once

#include <vector>
#include "Matrix4x4.h"
#include <wrl.h>
#include <d3d12.h>
#include <span>
#include "VertexInfluence.h"
#include "WellForGPU.h"

struct SkinningInformation {
    uint32_t numVertices;
};

struct SkinCluster {
    std::vector<Matrix4x4> inverseBindPoseMatrices;
    Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
    D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
    std::span<VertexInfluence> mappedInfluence;
    Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
    std::span<WellForGPU> mappedPalette;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;

    // コンピュートシェーダー用リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> skinningInformationResource;
    SkinningInformation* mappedSkinningInformation = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> skinnedVertexResource;
    D3D12_VERTEX_BUFFER_VIEW skinnedVertexBufferView;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> skinnedVertexSrvHandle;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> influenceSrvHandle;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> skinnedVertexUavHandle;
};