#include "RenderTexture.h"
#include "DirectXCommon.h"
#include "DescriptorPool.h"
#include "Engine/Manager/DrawManager.h"
#include <cassert>

void RenderTexture::Initialize(DirectXCommon* dxCommon, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor) {
    width_ = width;
    height_ = height;
    format_ = format;

    // リソースの作成
    resource_ = dxCommon->CreateRenderTextureResource(dxCommon->GetDevice(), width, height, format, clearColor);

    // RTVの作成
    rtvIndex_ = dxCommon->AllocateRTVIndex();
    rtvHandle_ = dxCommon->GetRTVCPUDescriptorHandle(rtvIndex_);
    
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    dxCommon->GetDevice()->CreateRenderTargetView(resource_.Get(), &rtvDesc, rtvHandle_);

    // SRVの作成
    srvIndex_ = dxCommon->GetSrvPool()->Allocate();
    srvHandleGPU_ = dxCommon->GetSrvPool()->GetGPUHandle(srvIndex_);
    
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    dxCommon->GetDevice()->CreateShaderResourceView(resource_.Get(), &srvDesc, dxCommon->GetSrvPool()->GetCPUHandle(srvIndex_));

    // 初期状態はレンダーターゲットだが、念のため SRV 状態へ即座に遷移させるなどの考慮は DrawManager 側で行う
    // (最初の BeginRenderTexture で StateBefore = PIXEL_SHADER_RESOURCE と矛盾しないようにするため)
}

// Draw メソッドは DrawManager を使用するように変更済み
void RenderTexture::Draw(DrawManager* drawManager) {
    if (!drawManager) return;
    drawManager->DrawRenderTexture(this);
}
