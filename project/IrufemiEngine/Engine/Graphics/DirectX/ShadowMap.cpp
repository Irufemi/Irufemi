#include "ShadowMap.h"
#include "DirectXCommon.h"
#include "DescriptorPool.h"
#include <cassert>

ShadowMap::~ShadowMap() {
    // SRV の解放
    if (dxCommon_ && dxCommon_->GetSrvPool()) {
        if (srvIndex_ != 0xFFFFFFFF) {
            dxCommon_->GetSrvPool()->FreeAfterFence(srvIndex_, dxCommon_->GetFenceValue());
        }
    }
    // DSV の解放
    if (dxCommon_ && dsvIndex_ != 0xFFFFFFFF) {
        dxCommon_->FreeDSVIndex(dsvIndex_);
    }
}

void ShadowMap::Initialize(DirectXCommon* dxCommon, uint32_t width, uint32_t height) {
    assert(dxCommon);
    dxCommon_ = dxCommon;
    ID3D12Device* device = dxCommon_->GetDevice();

    // 1. リソース作成
    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Width = width;
    resDesc.Height = height;
    resDesc.MipLevels = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // SRV と DSV で使い分けるため TYPELESS
    resDesc.SampleDesc.Count = 1;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearVal{};
    clearVal.DepthStencil.Depth = 1.0f;
    clearVal.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 最初は読み取り可能状態で生成
        &clearVal,
        IID_PPV_ARGS(resource_.GetAddressOf())
    );
    assert(SUCCEEDED(hr));

    // 2. DSV 作成
    dsvIndex_ = dxCommon_->AllocateDSVIndex();
    dsvHandleCPU_ = dxCommon_->GetDSVCPUDescriptorHandle(dsvIndex_);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(resource_.Get(), &dsvDesc, dsvHandleCPU_);

    // 3. SRV 作成
    srvIndex_ = dxCommon_->GetSrvPool()->Allocate();
    srvHandleCPU_ = dxCommon_->GetSrvPool()->GetCPUHandle(srvIndex_);
    srvHandleGPU_ = dxCommon_->GetSrvPool()->GetGPUHandle(srvIndex_);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(resource_.Get(), &srvDesc, srvHandleCPU_);

    // 4. ビューポート・シザーレクトの設定
    viewport_.Width = static_cast<float>(width);
    viewport_.Height = static_cast<float>(height);
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    scissorRect_.left = 0;
    scissorRect_.top = 0;
    scissorRect_.right = static_cast<LONG>(width);
    scissorRect_.bottom = static_cast<LONG>(height);
}

void ShadowMap::BeginRender(ID3D12GraphicsCommandList* commandList) {
    // ビューポートとシザーの設定
    commandList->RSSetViewports(1, &viewport_);
    commandList->RSSetScissorRects(1, &scissorRect_);

    // DSV のみをセット（カラーターゲットはなし）
    commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandleCPU_);
}

void ShadowMap::Clear(ID3D12GraphicsCommandList* commandList) {
    commandList->ClearDepthStencilView(dsvHandleCPU_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}
