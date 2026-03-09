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
#include "Renderer/LineInstanced/LineClass.h"
#include "Renderer/Skybox//Skybox.h"

#include "Renderer/D3D12ResourceUtil.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Resource/Model/ModelManager.h"
#include "Engine/Graphics/Data/CameraForGPU.h"
#include "Engine/Graphics/Data/DirectionalLight.h"
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

    // 各CBVのサイズを256バイトアラインメントに切り上げる
    const size_t cameraSize = (sizeof(CameraForGPU) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    const size_t directionalLightSize = (sizeof(DirectionalLight) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    const size_t pointLightsSize = (sizeof(PointLights) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    const size_t spotLightsSize = (sizeof(SpotLights) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    const size_t areaLightsSize = (sizeof(AreaLights) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);

    // フレームリソースを生成(全ライトとカメラを格納できるサイズ)
    const UINT frameResSize = static_cast<UINT>(cameraSize + directionalLightSize + pointLightsSize + spotLightsSize + areaLightsSize);
    frameResource_ = dxCommon_->CreateBufferResource(frameResSize);
    uint8_t* mapped = nullptr;
    frameResource_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

    // 各データのポインタとGPUアドレスをキャッシュ
    uintptr_t mappedAddress = reinterpret_cast<uintptr_t>(mapped);
    cameraData_ = reinterpret_cast<CameraForGPU*>(mappedAddress);
    directionalLightData_ = reinterpret_cast<DirectionalLight*>(mappedAddress + cameraSize);
    pointLightsData_ = reinterpret_cast<PointLights*>(mappedAddress + cameraSize + directionalLightSize);
    spotLightsData_ = reinterpret_cast<SpotLights*>(mappedAddress + cameraSize + directionalLightSize + pointLightsSize);
    areaLightsData_ = reinterpret_cast<AreaLights*>(mappedAddress + cameraSize + directionalLightSize + pointLightsSize + spotLightsSize);

    frameData_.camera = frameResource_->GetGPUVirtualAddress();
    frameData_.directionalLight = frameData_.camera + cameraSize;
    frameData_.pointLights = frameData_.directionalLight + directionalLightSize;
    frameData_.spotLights = frameData_.pointLights + pointLightsSize;
    frameData_.areaLights = frameData_.spotLights + spotLightsSize;
}

void DrawManager::Finalize() {
    if (frameResource_) {
        frameResource_->Unmap(0, nullptr);
        frameResource_.Reset();
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
    commandList_->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    commandList_->SetComputeRootSignature(dxCommon_->GetComputeRootSignature());
    commandList_->SetGraphicsRootConstantBufferView(5, frameData_.camera);
    commandList_->SetGraphicsRootConstantBufferView(3, frameData_.directionalLight);
    commandList_->SetGraphicsRootConstantBufferView(6, frameData_.pointLights);
    commandList_->SetGraphicsRootConstantBufferView(7, frameData_.spotLights);
    commandList_->SetGraphicsRootConstantBufferView(10, frameData_.areaLights);

    // 環境マップをバインド
    if (environmentMapHandle_.ptr != 0) {
        commandList_->SetGraphicsRootDescriptorTable(12, environmentMapHandle_);
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
    dxCommon_->GetSwapChain()->Present(1, 0);

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
    if (directionalLightData_) { *directionalLightData_ = light; }
    if (pointLightsData_) {
        for (int i = 0; i < kMaxPointLights; ++i) {
            pointLightsData_->lights[i].isActive = (i < pointLights.size());
            if (pointLightsData_->lights[i].isActive) {
                pointLightsData_->lights[i] = *pointLights[i];
            }
        }
    }
    if (spotLightsData_) {
        for (int i = 0; i < kMaxSpotLights; ++i) {
            spotLightsData_->lights[i].isActive = (i < spotLights.size());
            if (spotLightsData_->lights[i].isActive) {
                spotLightsData_->lights[i] = *spotLights[i];
            }
        }
    }
    if (areaLightsData_) {
        for (int i = 0; i < kMaxAreaLights; ++i) {
            areaLightsData_->lights[i].isActive = (i < areaLights.size());
            if (areaLightsData_->lights[i].isActive) {
                areaLightsData_->lights[i] = *areaLights[i];
            }
        }
    }
}

void DrawManager::SetEnvironmentMap(D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle) {
    environmentMapHandle_ = envMapHandle;
}

void DrawManager::DrawTriangle(
    TriangleClass* triangle
) {

    /*三角形を表示しよう*/
    //RootSignatureを設定。PSOに設定しているけど別途指定が必要
    commandList_->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    commandList_->IASetVertexBuffers(0, 1, &triangle->GetD3D12Resource()->vertexBufferView_); // VBVを設定
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

    /*三角形の色を変えよう*/

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    commandList_->SetGraphicsRootConstantBufferView(0, triangle->GetD3D12Resource()->materialResource_->GetGPUVirtualAddress());

    /*三角形を動かそう*/

    //wvp用のCBufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    commandList_->SetGraphicsRootConstantBufferView(1, triangle->GetD3D12Resource()->transformationResource_->GetGPUVirtualAddress());

    // ↓ ここから追加
    // GSパイプライン用のwvp CBufferをルートパラメータ[8]に設定
    commandList_->SetGraphicsRootConstantBufferView(8, triangle->GetD3D12Resource()->transformationResource_->GetGPUVirtualAddress());
    // ↑ ここまで追加

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    commandList_->SetGraphicsRootDescriptorTable(2, triangle->GetD3D12Resource()->textureHandle_);

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    commandList_->DrawInstanced(static_cast<UINT>(triangle->GetD3D12Resource()->vertexDataList_.size()), 1, 0, 0);

}

void DrawManager::DrawObject2D(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, const UINT& indexCount) {

    // 3. バッファ設定(VBV、IBV、Topology)

    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //VBVを設定
    commandList_->IASetVertexBuffers(0, 1, &vertexBufferView);
    //IBVを設定
    commandList_->IASetIndexBuffer(&indexBufferView);

    // 4. 定数バッファ(CBV)やライト用CBVの設定

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    commandList_->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

    //wvp用のCBufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    commandList_->SetGraphicsRootConstantBufferView(1, transformationResource->GetGPUVirtualAddress());

    // 5. テクスチャ用のDescriptor Table設定(SRV)

    /*テクスチャを貼ろう*/

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    commandList_->SetGraphicsRootDescriptorTable(2, textureHandle);

    // 6. 描画

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    commandList_->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);

}

void DrawManager::DrawParticle(ParticleSystem* resource) {

    // インスタンス数が0の場合は描画しない
    if (resource->GetInstanceCount() == 0) {
        return;
    }

    // IA 設定: VB/IB/Topology
    commandList_->IASetVertexBuffers(0, 1, &resource->GetD3D12Resource()->vertexBufferView_);
    commandList_->IASetIndexBuffer(&resource->GetD3D12Resource()->indexBufferView_);
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- CBV のバインド ---
    // 0: 既存のマテリアル CBV(互換性維持のために常にバインド)
    //    (rootParameters[0] に対応、PixelShader 側の b0 想定)
    commandList_->SetGraphicsRootConstantBufferView(0, resource->GetD3D12Resource()->materialResource_->GetGPUVirtualAddress());

    // インスタンス用 SRV (VS 側で参照するインスタンス配列)
    auto instancing = resource->GetInstancingSrvHandleGPU();
    assert(instancing.ptr != 0 && "Instancing SRV handle is null or invalid");
    commandList_->SetGraphicsRootDescriptorTable(4, resource->GetInstancingSrvHandleGPU());

    // テクスチャ (PS t0)
    commandList_->SetGraphicsRootDescriptorTable(2, resource->GetD3D12Resource()->textureHandle_);

    // 描画コール: インデックス数 × インスタンス数
    commandList_->DrawIndexedInstanced(
        static_cast<UINT>(resource->GetD3D12Resource()->indexDataList_.size()),
        resource->GetInstanceCount(),
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
    commandList_->SetGraphicsRootConstantBufferView(0, region->GetMaterialResource()->GetGPUVirtualAddress());
    commandList_->SetGraphicsRootDescriptorTable(2, region->GetTextureHandle());
    commandList_->SetGraphicsRootDescriptorTable(4, region->GetInstancingSrvHandleGPU());

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
    commandList_->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());          // PS b0

    // SRV (PS t0 / VS t0)
    commandList_->SetGraphicsRootDescriptorTable(2, textureHandle);            // PS t0
    commandList_->SetGraphicsRootDescriptorTable(4, instancingSrvHandleGPU);   // VS t0

    // Draw
    commandList_->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);
}

void DrawManager::DrawLineInstanced(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, const D3D12_GPU_DESCRIPTOR_HANDLE& instancingSrvHandleGPU, const UINT& instanceCount) {

    if (instanceCount == 0) return;

    // IA
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList_->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList_->IASetIndexBuffer(&indexBufferView);

    // SRV (VS t1)
    commandList_->SetGraphicsRootDescriptorTable(11, instancingSrvHandleGPU);

    // Draw
    commandList_->DrawIndexedInstanced(2, instanceCount, 0, 0, 0);
}

void DrawManager::DrawModel(const ManagedModel* model, D3D12_GPU_VIRTUAL_ADDRESS transformGpuVA, const std::vector<std::shared_ptr<GpuMaterial>>& materials) {
    if (!model || !model->cpuModel || !dxCommon_) return;

    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // モデル内の全メッシュをループして描画
    for (size_t i = 0; i < model->gpuMeshes.size(); ++i) {
        const auto& gpuMesh = model->gpuMeshes[i];
        const auto& gpuMaterial = (i < materials.size() && materials[i]) ? materials[i] : model->gpuMaterials[i];

        if (!gpuMesh || !gpuMaterial) continue;

        // IA (頂点/インデックス)
        commandList_->IASetVertexBuffers(0, 1, &gpuMesh->vertexBufferView);

        if (gpuMesh->indexCount > 0) {
            commandList_->IASetIndexBuffer(&gpuMesh->indexBufferView);
        }

        // CBV (マテリアル/Transform)
        commandList_->SetGraphicsRootConstantBufferView(0, gpuMaterial->materialResource->GetGPUVirtualAddress());
        commandList_->SetGraphicsRootConstantBufferView(1, transformGpuVA);

        // SRV (テクスチャ)
        commandList_->SetGraphicsRootDescriptorTable(2, gpuMaterial->textureHandle);

        // 描画コマンド
        if (gpuMesh->indexCount > 0) {
            commandList_->DrawIndexedInstanced(gpuMesh->indexCount, 1, 0, 0, 0);
        } else {
            commandList_->DrawInstanced(gpuMesh->vertexCount, 1, 0, 0);
        }
    }
}

void DrawManager::DrawAnimationModel(const ManagedModel* model, D3D12_GPU_VIRTUAL_ADDRESS transformGpuVA, const SkinCluster& skinCluster, const D3D12_GPU_DESCRIPTOR_HANDLE& skinnedVertexSrv, D3D12_GPU_VIRTUAL_ADDRESS skinningInfoGpuVA, uint32_t numVertices, const std::vector<std::shared_ptr<GpuMaterial>>& materials)
{
    if (!model || !model->cpuModel || !dxCommon_) return;

    // --- コンピュートシェーダーによるスキニング実行 ---
    // PSOをコンピュート用に切り替え
    commandList_->SetPipelineState(dxCommon_->GetSkinningComputePSO());

    // Parameterの設定
    commandList_->SetComputeRootDescriptorTable(0, skinCluster.paletteSrvHandle.second);
    commandList_->SetComputeRootDescriptorTable(1, model->gpuMeshes[0]->vertexSrvHandle); // 入力頂点(t1)
    commandList_->SetComputeRootDescriptorTable(2, skinCluster.influenceSrvHandle.second);
    commandList_->SetComputeRootDescriptorTable(3, skinCluster.skinnedVertexUavHandle.second);
    commandList_->SetComputeRootConstantBufferView(4, skinningInfoGpuVA);

    // Dispatch
    commandList_->Dispatch((numVertices + 1023) / 1024, 1, 1);

    // --- UAVバリア ---
    // スキニング計算結果(UAV)を、頂点シェーダーの入力(SRV)として使えるようにするためのバリア
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = skinCluster.skinnedVertexResource.Get();
    commandList_->ResourceBarrier(1, &barrier);

    // --- グラフィックスパイプラインでの描画 ---
    // PSOをグラフィックス用に切り替え (呼び出し元で ApplyPSO が呼ばれている想定)
    commandList_->SetPipelineState(dxCommon_->GetPSOManager()->Get(BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));

    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Skinning用のMatrixPaletteをSRVとしてバインド
    // (VSでも参照する可能性があるため、ここではコメントアウトせず残す)
    commandList_->SetGraphicsRootDescriptorTable(4, skinCluster.paletteSrvHandle.second);

    // モデル内の全メッシュをループして描画
    for (size_t i = 0; i < model->gpuMeshes.size(); ++i) {
        const auto& gpuMesh = model->gpuMeshes[i];
        const auto& gpuMaterial = (i < materials.size() && materials[i]) ? materials[i] : model->gpuMaterials[i];

        if (!gpuMesh || !gpuMaterial) continue;

        // セットするバッファの配列を用意
        D3D12_VERTEX_BUFFER_VIEW vbvs[1];
        UINT vbvCount = 0;

        // 1. コンピュートシェーダーで計算済みの頂点バッファをセット
        vbvs[vbvCount] = skinCluster.skinnedVertexBufferView;
        vbvCount++;

        // 実際に存在するバッファの数（1つ）だけをGPUに教える
        commandList_->IASetVertexBuffers(0, vbvCount, vbvs);

        if (gpuMesh->indexCount > 0) {
            commandList_->IASetIndexBuffer(&gpuMesh->indexBufferView);
        }

        // CBV (マテリアル/Transform)
        commandList_->SetGraphicsRootConstantBufferView(0, gpuMaterial->materialResource->GetGPUVirtualAddress());
        commandList_->SetGraphicsRootConstantBufferView(1, transformGpuVA);

        // SRV (テクスチャ)
        commandList_->SetGraphicsRootDescriptorTable(2, gpuMaterial->textureHandle);

        // 描画コマンド
        if (gpuMesh->indexCount > 0) {
            commandList_->DrawIndexedInstanced(gpuMesh->indexCount, 1, 0, 0, 0);
        } else {
            commandList_->DrawInstanced(gpuMesh->vertexCount, 1, 0, 0);
        }
    }
}

void DrawManager::DrawSkybox(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, const UINT& indexCount) {

    commandList_->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定
    //IBVを設定
    commandList_->IASetIndexBuffer(&indexBufferView);
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    commandList_->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

    //wvp用のCBufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    commandList_->SetGraphicsRootConstantBufferView(1, transformationResource->GetGPUVirtualAddress());

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    commandList_->SetGraphicsRootDescriptorTable(2, textureHandle);

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。
    commandList_->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void DrawManager::DrawObject3D(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, const UINT& indexCount) {
    /*三角形を表示しよう*/
    //RootSignatureを設定。PSOに設定しているけど別途指定が必要
    commandList_->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定
    //IBVを設定
    commandList_->IASetIndexBuffer(&indexBufferView);
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    /*三角形の色を変えよう*/

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    commandList_->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

    /*三角形を動かそう*/

    //wvp用のCBufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    commandList_->SetGraphicsRootConstantBufferView(1, transformationResource->GetGPUVirtualAddress());

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    commandList_->SetGraphicsRootDescriptorTable(2, textureHandle);

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    commandList_->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void DrawManager::DispatchSkinning(const D3D12_GPU_DESCRIPTOR_HANDLE& palette, const D3D12_GPU_DESCRIPTOR_HANDLE& inputVertex, const D3D12_GPU_DESCRIPTOR_HANDLE& influence, const D3D12_GPU_DESCRIPTOR_HANDLE& outputVertex, const D3D12_GPU_VIRTUAL_ADDRESS& skinningInformation, const float& verticesSize) {


    // Parameterの設定
    commandList_->SetComputeRootDescriptorTable(0, palette);
    commandList_->SetComputeRootDescriptorTable(1, inputVertex);
    commandList_->SetComputeRootDescriptorTable(2, influence);
    commandList_->SetComputeRootDescriptorTable(3, outputVertex);
    commandList_->SetComputeRootConstantBufferView(4, skinningInformation);

    // Dispatch
    commandList_->Dispatch(static_cast<UINT>(verticesSize + 1023 / 1024), 1, 1);

}

void DrawManager::DrawParticleGPU(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_GPU_VIRTUAL_ADDRESS& perView, const D3D12_GPU_VIRTUAL_ADDRESS& material, const D3D12_GPU_DESCRIPTOR_HANDLE& particleSrv, const D3D12_GPU_DESCRIPTOR_HANDLE& textureHandle, const UINT& instanceCount) {

    // IA 設定: VB/IB/Topology
    commandList_->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- CBV のバインド ---
    // (rootParameters[0] に対応、PixelShader 側の b0 想定)
    commandList_->SetGraphicsRootConstantBufferView(0, perView);

    commandList_->SetGraphicsRootConstantBufferView(1, material);

    // --- SRVのバインド ---
    // テクスチャ (PS t0)
    commandList_->SetGraphicsRootDescriptorTable(2, textureHandle);
    // パーティクルデータ (VS t0)
    commandList_->SetGraphicsRootDescriptorTable(4, particleSrv);

    // 描画コール
    commandList_->DrawInstanced(6, instanceCount, 0, 0);
}