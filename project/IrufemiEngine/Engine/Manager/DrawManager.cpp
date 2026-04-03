#include "DrawManager.h"

#include<Windows.h>
#include <cassert>

#include <dxgidebug.h>
#include "Renderer/Object2D/Sprite/Sprite.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"
#include "Renderer/Object3D/Primitive/SphereClass.h"
#include "Renderer/Object3D/Primitive/TriangleClass.h"
#include "Renderer/Object3D/Primitive/CylinderClass.h"
#include "Renderer/Object3D/Primitive/CubeClass.h"
#include "Renderer/Region/Region.h"
#include "Renderer/Region/Primitive/SphereRegion.h"
#include "Renderer/Region/Primitive/TetraRegion.h"
#include "Renderer/Particle/ParticleSystem.h"
#include "Renderer/Particle/ParticleResource.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Renderer/Skybox//Skybox.h"
#include "Renderer/Object3D/Object3DResource.h"
#include "Renderer/Object2D/Object2DResource.h"

#include "Renderer/LineInstanced/LineResource.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Resource/Model/ModelManager.h"
#include "Engine/Graphics/Data/CameraForGPU.h"
#include "Engine/Graphics/Data/DirectionalLight.h"
#include "Engine/Graphics/Data/PointLight.h"
#include "Engine/Graphics/Data/SpotLight.h"
#include "Engine/Graphics/Data/AreaLight.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include "Resource/Model/Data/SkinCluster.h"


namespace {
    // cpp内限定のヌルCBV
    Microsoft::WRL::ComPtr<ID3D12Resource> gNullPointLight;
    Microsoft::WRL::ComPtr<ID3D12Resource> gNullSpotLight;
    D3D12_GPU_VIRTUAL_ADDRESS gNullPointLightVA = 0;
    D3D12_GPU_VIRTUAL_ADDRESS gNullSpotLightVA = 0;

    void EnsureNullPointLight(DirectXCommon* dx) {
        if (gNullPointLight) return;
        gNullPointLight = dx->CreateBufferResource(sizeof(PointLight));
        PointLight* p = nullptr;
        gNullPointLight->Map(0, nullptr, reinterpret_cast<void**>(&p));
        p->color = { 0,0,0,0 }; p->position = { 0,0,0 }; p->intensity = 0.0f;
        gNullPointLightVA = gNullPointLight->GetGPUVirtualAddress();
    }

    void EnsureNullSpotLight(DirectXCommon* dx) {
        if (gNullSpotLight) return;
        gNullSpotLight = dx->CreateBufferResource(sizeof(SpotLight));
        SpotLight* s = nullptr;
        gNullSpotLight->Map(0, nullptr, reinterpret_cast<void**>(&s));
        s->color = { 0,0,0,0 };
        s->position = { 0,0,0 };
        s->intensity = 0.0f;
        s->direction = { 0, -1, 0 };
        s->distance = 0.0f;
        s->decay = 1.0f;
        s->cosAngle = 1.0f;
        gNullSpotLightVA = gNullSpotLight->GetGPUVirtualAddress();
    }
} // anonymous

void DrawManager::Initialize(DirectXCommon* dx) {
    dxCommon_ = dx;
    commandList_ = dx->GetCommandList();

    // 定数バッファのサイズ (256バイトアラインメント)
    const size_t cameraSize = (sizeof(CameraForGPU) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    const size_t lightCommonSize = (sizeof(LightCommonData) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);

    // フレーム定数バッファ (Camera + LightCommonData)
    frameResource_ = dxCommon_->CreateBufferResource(cameraSize + lightCommonSize);
    uint8_t* mapped = nullptr;
    frameResource_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

    cameraData_ = reinterpret_cast<CameraForGPU*>(mapped);
    lightCommonData_ = reinterpret_cast<LightCommonData*>(mapped + cameraSize);

    frameData_.camera = frameResource_->GetGPUVirtualAddress();
    frameData_.lightCommon = frameData_.camera + cameraSize;

    // StructuredBuffer の初期化 (ひとまず1024個分を確保)
    const uint32_t kMaxLights = 1024;
    pointLightResource_ = dxCommon_->CreateBufferResource(sizeof(PointLight) * kMaxLights);
    spotLightResource_ = dxCommon_->CreateBufferResource(sizeof(SpotLight) * kMaxLights);
    areaLightResource_ = dxCommon_->CreateBufferResource(sizeof(AreaLight) * kMaxLights);

    // ライト SRV デスクリプタの一括確保 (Point, Spot, Area 用に 3 つ)
    auto pool = dxCommon_->GetSrvPool();
    lightSrvBaseIndex_ = pool->Allocate(3);
    lightSrvHandle_ = pool->GetGPUHandle(lightSrvBaseIndex_);

    // StructuredBuffer SRV の作成
    pool->CreateSRVForStructuredBuffer(lightSrvBaseIndex_ + 0, pointLightResource_.Get(), kMaxLights, sizeof(PointLight));
    pool->CreateSRVForStructuredBuffer(lightSrvBaseIndex_ + 1, spotLightResource_.Get(), kMaxLights, sizeof(SpotLight));
    pool->CreateSRVForStructuredBuffer(lightSrvBaseIndex_ + 2, areaLightResource_.Get(), kMaxLights, sizeof(AreaLight));
}

void DrawManager::Finalize() {
    if (frameResource_) {
        frameResource_->Unmap(0, nullptr);
        frameResource_.Reset();
    }
    pointLightResource_.Reset();
    spotLightResource_.Reset();
    areaLightResource_.Reset();

    // SRVの解放
    auto* srvPool = dxCommon_->GetSrvPool();
    uint64_t fv = dxCommon_->GetFenceValue();
    if (srvPool && lightSrvBaseIndex_ != 0xFFFFFFFFu) {
        // 連続した3つのデスクリプタを個別に返却 (Freeは1つずつ用のため)
        for (uint32_t i = 0; i < 3; ++i) {
            srvPool->FreeAfterFence(lightSrvBaseIndex_ + i, fv);
        }
        lightSrvBaseIndex_ = 0xFFFFFFFFu;
    }

    dxCommon_ = nullptr;
    commandList_ = nullptr;
}

void DrawManager::BindPSO(ID3D12PipelineState* pso) {
    if (pso) {
        commandList_->SetPipelineState(pso);
    }
}

void DrawManager::PreDraw(std::array<float, 4> clearColor, float clearDepth, uint8_t clearStencil) {

    // バックバッファとRTV/DSVの取得
    const UINT backIdx = dxCommon_->GetSwapChain()->GetCurrentBackBufferIndex();
    ID3D12Resource* backBuffer = dxCommon_->GetSwapChainResources(backIdx);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->GetRtvHandles(backIdx);
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDSVCPUDescriptorHandle(0);

    /*完璧な画面クリアを目指して*/

    ///TransitionBarrierを張るコード

    //TransitionBarrierの設定
    D3D12_RESOURCE_BARRIER barrier{};
    //今回のバリアはTransition
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    //Noneにしておく
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    //バリアを張る対象のリソース。現在のバックバッファに対して行う
    barrier.Transition.pResource = backBuffer;
    // リソースバリアの SubResource を D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES にして明示
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    //遷移前(現在)のResourceState
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    //遷移後のResourceState
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    //TransitionBarrierを張る
    commandList_->ResourceBarrier(1, &barrier);

    /*画面の色を変えよう*/

    ///コマンドを積み込んで確定させる

    //描画先のRTVを設定する
    commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
    //指定した色で画面全体をクリアする
    commandList_->ClearRenderTargetView(rtvHandle, clearColor.data(), 0, nullptr);

    /*前後関係を正しくしよう*/

    //指定した深度で画面全体をクリアする
    commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, clearDepth, clearStencil, 0, nullptr);

    // フレーム共通のビューポート/シザーを一度だけ設定
    commandList_->RSSetViewports(1, &dxCommon_->GetViewport());
    commandList_->RSSetScissorRects(1, &dxCommon_->GetScissorRect());

    // フレームで利用するSRVヒープを設定(全描画共通)
    ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
    commandList_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // --- フレーム共通CBV/SRVをここで一度だけバインド ---
    BindCommonParameters();

    // 環境マップをバインド
    if (environmentMapHandle_.ptr != 0) {
        commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::EnvMap, environmentMapHandle_);
    }
}

void DrawManager::PostDraw() {

    const UINT backIdx = dxCommon_->GetSwapChain()->GetCurrentBackBufferIndex();
    ID3D12Resource* backBuffer = dxCommon_->GetSwapChainResources(backIdx);

    D3D12_RESOURCE_BARRIER barrier{};

    /*完璧な画面クリアを目指して*/

    //今回のバリアはTransition
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    //Noneにしておく
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    //バリアを張る対象のリソース。現在のバックバッファに対して行う
    barrier.Transition.pResource = backBuffer;
    // リソースバリアの SubResource を D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES にして明示
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    //画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    //今回はRenderTargetからPresentにする
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    //TransitionBarrierを張る
    commandList_->ResourceBarrier(1, &barrier);

    /*画面の色を変えよう*/

    ///コマンドを積み込んで確定させる

    //コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
    HRESULT hr = commandList_->Close();
    assert(SUCCEEDED(hr));

    ///コマンドをキックする

    //GPUにコマンドリストの実行を行わせる
    ID3D12CommandList* commandLists[] = { commandList_ };
    dxCommon_->GetCommandQueue()->ExecuteCommandLists(_countof(commandLists), commandLists);
    //GPUとOSに画面の交換を行うよう通知する
    hr = dxCommon_->GetSwapChain()->Present(1, 0);
    // デバイスが削除されたかどうかのチェック
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_DEVICE_REMOVED) {
            HRESULT removedReason = dxCommon_->GetDevice()->GetDeviceRemovedReason();
            char str[256];
            sprintf_s(str, "Device Removed, reason code: 0x%08X\n", removedReason);
            OutputDebugStringA(str);
        }
        // 他のエラーコードも必要に応じて処理
        assert(SUCCEEDED(hr));
    }


    /*完璧な画面クリアを目指して*/

    ///GPUにSignal(シグナル)を送る

    //Fenceの値を更新
    uint64_t& fenceValue = dxCommon_->GetFenceValue();
    fenceValue++;
    //GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
    dxCommon_->GetCommandQueue()->Signal(dxCommon_->GetFence(), fenceValue);

    ///Fenceの値を確認してGPUを待つ

    //Fenceの値が指定したSignal値にたどり着いているか確認する
    //GetCompletedValueの初期値はFence作成時に渡した初期値
    if (dxCommon_->GetFence()->GetCompletedValue() < fenceValue) {
        //指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
        dxCommon_->GetFence()->SetEventOnCompletion(fenceValue, dxCommon_->GetFenceEvent());
        //イベント待つ
        WaitForSingleObject(dxCommon_->GetFenceEvent(), INFINITE);
    }

    dxCommon_->UpdateFixFPS();

    /*画面の色を変えよう*/

    ///コマンドを積み込んで確定させる

    //次のフレーム用のコマンドリストを準備
    hr = dxCommon_->GetCommandAllocator()->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList_->Reset(dxCommon_->GetCommandAllocator(), nullptr);
    assert(SUCCEEDED(hr));

}

void DrawManager::SetFrameData(const CameraForGPU& camera, const DirectionalLight& light, const std::vector<PointLight*>& pointLights, const std::vector<SpotLight*>& spotLights, const std::vector<AreaLight*>& areaLights) {
    if (cameraData_) { *cameraData_ = camera; }
    if (lightCommonData_) {
        // ライト共通データの更新（b1）
        lightCommonData_->directionalLight = light;
        lightCommonData_->pointLightCount = static_cast<int32_t>(pointLights.size());
        lightCommonData_->spotLightCount = static_cast<int32_t>(spotLights.size());
        lightCommonData_->areaLightCount = static_cast<int32_t>(areaLights.size());
    }

    // 各 StructuredBuffer へ書き込み
    auto copyLights = [](ID3D12Resource* res, const auto& lightVec) {
        if (!res || lightVec.empty()) return;
        using LightType = std::remove_pointer_t<typename std::decay_t<decltype(lightVec)>::value_type>;
        LightType* mapped = nullptr;
        res->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
        for (size_t i = 0; i < lightVec.size(); ++i) {
            mapped[i] = *lightVec[i];
        }
        res->Unmap(0, nullptr);
    };

    copyLights(pointLightResource_.Get(), pointLights);
    copyLights(spotLightResource_.Get(), spotLights);
    copyLights(areaLightResource_.Get(), areaLights);
}

void DrawManager::SetEnvironmentMap(D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle) {
    environmentMapHandle_ = envMapHandle;
}


void DrawManager::DrawObject2D(const Object2DResource* resource) {
    if (!resource) return;

    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList_->IASetVertexBuffers(0, 1, &resource->vertexBufferView_);
    commandList_->IASetIndexBuffer(&resource->indexBufferView_);

    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, resource->materialResource_->GetGPUVirtualAddress());
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Transform, resource->transformationResource_->GetGPUVirtualAddress());
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, resource->textureHandle_);

    commandList_->DrawIndexedInstanced(resource->indexCount_, 1, 0, 0, 0);
}

void DrawManager::DrawParticle(const ParticleResource* resource, uint32_t instanceCount) {
    // インスタンス数が0の場合は描画しない
    if (instanceCount == 0) {
        return;
    }

    // IA 設定: VB/IB/Topology
    commandList_->IASetVertexBuffers(0, 1, &resource->vertexBufferView_);
    commandList_->IASetIndexBuffer(&resource->indexBufferView_);
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- CBV のバインド ---
    // 0: 既存のマテリアル CBV(互換性維持のために常にバインド)
    //    (rootParameters[(UINT)RootSlot::Material] に対応、PixelShader 側の b0 想定)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, resource->materialResource_->GetGPUVirtualAddress());

    // インスタンス用 SRV (VS 側で参照するインスタンス配列)
    assert(resource->instancingSrvHandleGPU_.ptr != 0 && "Instancing SRV handle is null or invalid");
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Instancing, resource->instancingSrvHandleGPU_);

    // テクスチャ (PS t0)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, resource->textureHandle_);

    // 描画コール: インデックス数 × インスタンス数
    commandList_->DrawIndexedInstanced(
        resource->indexCount_,
        instanceCount,
        0, 0, 0
    );
}

void DrawManager::DrawModelRegion(ModelRegion* region) {
    if (!region) { return; }
    const GpuMesh* gpuMesh = region->GetGpuMesh();
    if (!gpuMesh || gpuMesh->vertexCount == 0 || region->GetInstanceCount() == 0) { return; }

    // IA設定 (共有リソースから)
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList_->IASetVertexBuffers(0, 1, &gpuMesh->vertexBufferView);
    if (gpuMesh->indexCount > 0) {
        commandList_->IASetIndexBuffer(&gpuMesh->indexBufferView);
    }

    // CBV/SRV設定 (インスタンスリソースから)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, region->GetMaterialResource()->GetGPUVirtualAddress());
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, region->GetTextureHandle());
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Instancing, region->GetInstancingSrvHandleGPU());

    // 描画
    if (gpuMesh->indexCount > 0) {
        commandList_->DrawIndexedInstanced(gpuMesh->indexCount, region->GetInstanceCount(), 0, 0, 0);
    } else {
        commandList_->DrawInstanced(gpuMesh->vertexCount, region->GetInstanceCount(), 0, 0);
    }
}

void DrawManager::DrawRegion(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, const D3D12_GPU_DESCRIPTOR_HANDLE& textureHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& instancingSrvHandleGPU, const UINT& indexCount, const UINT& instanceCount) {

    if (indexCount == 0 || instanceCount == 0) { return; }

    // IA
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList_->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList_->IASetIndexBuffer(&indexBufferView);

    // CBV (PS)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, materialResource->GetGPUVirtualAddress());          // PS b0

    // SRV (PS t0 / VS t0)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, textureHandle);            // PS t0
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Instancing, instancingSrvHandleGPU);   // VS t0

    // Draw
    commandList_->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);
}

void DrawManager::DrawLineInstanced(const LineResource* resource, const D3D12_GPU_DESCRIPTOR_HANDLE& instancingSrvHandleGPU, const UINT& instanceCount) {
    if (!resource || instanceCount == 0) return;

    // IA
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList_->IASetVertexBuffers(0, 1, &resource->vertexBufferView_);
    commandList_->IASetIndexBuffer(&resource->indexBufferView_);

    // SRV (VS t1)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::LineInstancing, instancingSrvHandleGPU);

    // Draw
    commandList_->DrawIndexedInstanced(2, instanceCount, 0, 0, 0);
}

void DrawManager::DispatchSkinning(const SkinCluster& skinCluster, const ManagedModel* model, uint32_t numVertices) {
    if (!model || !model->gpuMeshes[0] || !dxCommon_) return;

    // --- コンピュートシェーダーによるスキニング実行 ---
    // PSOをコンピュート用に切り替え
    commandList_->SetPipelineState(dxCommon_->GetSkinningComputePSO());

    // RootSignatureはSkipして共通のComputeRootSignatureを使用する想定
    // (PSO設定時にセットされているはずだが、念のため管理が必要な場合はここでセット)

    // Parameterの設定
    // 0: Palette (t0)
    commandList_->SetComputeRootDescriptorTable(0, skinCluster.paletteSrvHandle.second);
    // 1: Input Vertices (t1) (最初のメッシュの頂点を使用)
    commandList_->SetComputeRootDescriptorTable(1, model->gpuMeshes[0]->vertexSrvHandle);
    // 2: Influences (t2)
    commandList_->SetComputeRootDescriptorTable(2, skinCluster.influenceSrvHandle.second);
    // 3: Output Vertices (u0)
    commandList_->SetComputeRootDescriptorTable(3, skinCluster.skinnedVertexUavHandle.second);
    // 4: Skinning Information (b0)
    commandList_->SetComputeRootConstantBufferView(4, skinCluster.skinningInformationResource->GetGPUVirtualAddress());

    // Dispatch
    commandList_->Dispatch((numVertices + 1023) / 1024, 1, 1);
}

void DrawManager::ExecuteUAVBarrier(ID3D12Resource* resource) {
    if (!resource) return;
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = resource;
    commandList_->ResourceBarrier(1, &barrier);
}

void DrawManager::DrawSkybox(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, const UINT& indexCount) {

    commandList_->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定
    //IBVを設定
    commandList_->IASetIndexBuffer(&indexBufferView);
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, materialResource->GetGPUVirtualAddress());

    //wvp用のCBufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Transform, transformationResource->GetGPUVirtualAddress());

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, textureHandle);

    //描画！（DrawCall/ドローコール）。3頂点で1つのインスタンス。
    commandList_->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void DrawManager::DrawObject3D(const Object3DResource* resource) {
    if (!resource) return;
    
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList_->IASetVertexBuffers(0, 1, &resource->vertexBufferView_);
    commandList_->IASetIndexBuffer(&resource->indexBufferView_);

    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, resource->materialResource_->GetGPUVirtualAddress());
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Transform, resource->transformationResource_->GetGPUVirtualAddress());
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, resource->textureHandle_);

    commandList_->DrawIndexedInstanced(resource->indexCount_, 1, 0, 0, 0);
}


void DrawManager::DrawParticleGPU(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_GPU_VIRTUAL_ADDRESS& perView, const D3D12_GPU_VIRTUAL_ADDRESS& material, const D3D12_GPU_DESCRIPTOR_HANDLE& particleSrv, const D3D12_GPU_DESCRIPTOR_HANDLE& textureHandle, const UINT& instanceCount) {

    // IA 設定: VB/IB/Topology
    commandList_->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- CBV のバインド ---
    // (rootParameters[(UINT)RootSlot::Material] に対応、PixelShader 側の b0 想定)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, perView);

    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Transform, material);

    // --- SRVのバインド ---
    // テクスチャ (PS t0)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, textureHandle);
    // パーティクルデータ (VS t0)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Instancing, particleSrv);

    // 描画コール
    commandList_->DrawInstanced(6, instanceCount, 0, 0);
}
void DrawManager::BeginRenderTexture(RenderTexture* rt, const Vector4& clearColor) {
    // 1. Transition Barrier (SRV -> RenderTarget)
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = rt->GetResource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList_->ResourceBarrier(1, &barrier);

    // 2. Set Render Target
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rt->GetRtvHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDSVCPUDescriptorHandle(0);
    commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

    // 3. Clear
    commandList_->ClearRenderTargetView(rtvHandle, &clearColor.x, 0, nullptr);
    commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // 4. Set Viewport/Scissor
    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(rt->GetWidth()), static_cast<float>(rt->GetHeight()), 0.0f, 1.0f };
    D3D12_RECT scissor{ 0, 0, static_cast<long>(rt->GetWidth()), static_cast<long>(rt->GetHeight()) };
    commandList_->RSSetViewports(1, &viewport);
    commandList_->RSSetScissorRects(1, &scissor);

    // 5. Descriptor Heaps (念のため再設定)
    ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
    commandList_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
}

void DrawManager::EndRenderTexture(RenderTexture* rt) {
    // 1. Transition Barrier (RenderTarget -> SRV)
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = rt->GetResource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList_->ResourceBarrier(1, &barrier);
}

void DrawManager::SetRenderTargetToBackBuffer(bool useDepth) {
    const UINT backIdx = dxCommon_->GetSwapChain()->GetCurrentBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->GetRtvHandles(backIdx);
    if (useDepth) {
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDSVCPUDescriptorHandle(0);
        commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
    } else {
        commandList_->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
    }

    // ビューポートとシザーを元に戻す
    commandList_->RSSetViewports(1, &dxCommon_->GetViewport());
    commandList_->RSSetScissorRects(1, &dxCommon_->GetScissorRect());
}

void DrawManager::DrawRenderTexture(RenderTexture* renderTexture, ID3D12PipelineState* pso, D3D12_GPU_VIRTUAL_ADDRESS cbvAddress, D3D12_GPU_DESCRIPTOR_HANDLE depthSrvHandle) {
    if (!renderTexture) return;

    // 1. PSOの設定 (引数が渡された場合はそれを使用、そうでなければデフォルトのCopyImage)
    if (pso) {
        commandList_->SetPipelineState(pso);
    } else {
        ID3D12PipelineState* defaultPso = dxCommon_->GetPSOManager()->GetCopyImage();
        if (!defaultPso) return;
        commandList_->SetPipelineState(defaultPso);
    }

    // 2. ルートシグネチャの設定
    commandList_->SetGraphicsRootSignature(dxCommon_->GetRootSignature());

    // 3. 形状の設定 (三角形リスト)
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 4. テクスチャの設定 (RootParameter[(UINT)RootSlot::Texture])
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, renderTexture->GetSrvHandleGPU());

    // 深度テクスチャの設定 (RootParameter[(UINT)RootSlot::EnvMap])
    if (depthSrvHandle.ptr != 0) {
        commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::EnvMap, depthSrvHandle);
    }

    // 追加: ConstantBuffer の設定 (引数があれば RootParameter[(UINT)RootSlot::Material] にセット)
    if (cbvAddress != 0) {
        commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, cbvAddress);
    }

    // 5. 描画 (3頂点のインデックスなし描画: SV_VertexIDを使用するためVBいらず)
    commandList_->DrawInstanced(3, 1, 0, 0);
}
void DrawManager::BindCommonParameters() {
    if (!commandList_ || !dxCommon_) return;

    commandList_->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    commandList_->SetComputeRootSignature(dxCommon_->GetComputeRootSignature());
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Camera, frameData_.camera);
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::LightCommon, frameData_.lightCommon);

    // 点光源、スポットライト、面光源を１つのテーブル（Slot 6）で一括設定
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Lights, lightSrvHandle_);
}
