#include "DrawManager.h"

#include<Windows.h>
#include <cassert>

#include <dxgidebug.h>
#include "3D/SphereClass.h"
#include "2D/Sprite.h"
#include "2D/SpriteRegion.h"
#include "3D/ObjClass.h"
#include "3D/TriangleClass.h"
#include "3D/particle/ParticleSystem.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "3D/CylinderClass.h"
#include "3D/Region.h"
#include "3D/SphereRegion.h"
#include "3D/TetraRegion.h"
#include "3D/LineClass.h"
#include "3D/CubeClass.h"

#include "source/D3D12ResourceUtil.h"
#include "engine/directX/DirectXCommon.h"
#include "manager/ModelManager.h" // GpuMeshのため
#include "math/CameraForGPU.h"
#include "math/DirectionalLight.h"
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "function/Math.h"


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

    // 各CBVのサイズを256バイトアラインメントに切り上げる
    const size_t cameraSize = (sizeof(CameraForGPU) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    const size_t directionalLightSize = (sizeof(DirectionalLight) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    const size_t pointLightSize = (sizeof(PointLight) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    const size_t spotLightSize = (sizeof(SpotLight) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);

    // フレームリソースを生成（全ライトとカメラを格納できるサイズ）
    const UINT frameResSize = static_cast<UINT>(cameraSize + directionalLightSize + pointLightSize + spotLightSize);
    frameResource_ = dxCommon_->CreateBufferResource(frameResSize);
    uint8_t* mapped = nullptr;
    frameResource_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

    // 各データのポインタとGPUアドレスをキャッシュ
    uintptr_t mappedAddress = reinterpret_cast<uintptr_t>(mapped);
    cameraData_ = reinterpret_cast<CameraForGPU*>(mappedAddress);
    directionalLightData_ = reinterpret_cast<DirectionalLight*>(mappedAddress + cameraSize);
    pointLightData_ = reinterpret_cast<PointLight*>(mappedAddress + cameraSize + directionalLightSize);
    spotLightData_ = reinterpret_cast<SpotLight*>(mappedAddress + cameraSize + directionalLightSize + pointLightSize);

    frameData_.camera = frameResource_->GetGPUVirtualAddress();
    frameData_.directionalLight = frameData_.camera + cameraSize;
    frameData_.pointLight = frameData_.directionalLight + directionalLightSize;
    frameData_.spotLight = frameData_.pointLight + pointLightSize;
}

void DrawManager::Finalize() {
    if (frameResource_) {
        frameResource_->Unmap(0, nullptr);
        frameResource_.Reset();
    }
    dxCommon_ = nullptr;
}

void DrawManager::BindPSO(ID3D12PipelineState* pso) {
    if (!pso) { return; }
    dxCommon_->GetCommandList()->SetPipelineState(pso);
}

void DrawManager::PreDraw(std::array<float, 4> clearColor, float clearDepth, uint8_t clearStencil) {

    // バックバッファとRTV/DSVの取得
    const UINT backIdx = dxCommon_->GetSwapChain()->GetCurrentBackBufferIndex();
    ID3D12Resource* backBuffer = dxCommon_->GetSwapChainResources(backIdx);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->GetRtvHandles(backIdx);
    auto* dsvHeap = dxCommon_->GetDsvDescriptorHeap();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

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
    // リソースバリアの Subresource を D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES にして明示
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    //遷移前(現在)のResourceState
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    //遷移後のResourceState
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    //TransitionBarrierを張る
    dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);

    /*画面の色を変えよう*/

    ///コマンドを積み込んで確定させる

    //描画先のRTVを設定する
    dxCommon_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
    //指定した色で画面全体をクリアする
    dxCommon_->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor.data(), 0, nullptr);

    /*前後関係を正しくしよう*/

    ///DSVを設定する

    //描画先のRTVとDSVを設定する
    dxCommon_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

    //指定した深度で画面全体をクリアする
    dxCommon_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, clearDepth, clearStencil, 0, nullptr);

    // フレーム共通のビューポート/シザーを一度だけ設定
    auto& viewport = dxCommon_->GetViewport();
    auto& scissorRect = dxCommon_->GetScissorRect();
    dxCommon_->GetCommandList()->RSSetViewports(1, &viewport);
    dxCommon_->GetCommandList()->RSSetScissorRects(1, &scissorRect);

    // フレームで利用するSRVヒープを設定（全描画共通）
    ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
    dxCommon_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);

    // --- フレーム共通CBVをここで一度だけバインド ---
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, frameData_.camera);
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, frameData_.directionalLight);
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, frameData_.pointLight);
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(7, frameData_.spotLight);

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
    // リソースバリアの Subresource を D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES にして明示
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    //画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    //今回はRenderTargetからPresentにする
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    //TransitionBarrierを張る                                                                                                                                                                                                                                                                                                                                                                                                                                                                rrierを張る
    dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);

    /*画面の色を変えよう*/

    ///コマンドを積み込んで確定させる

    //コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
    HRESULT hr = dxCommon_->GetCommandList()->Close();
    assert(SUCCEEDED(hr));

    ///コマンドをキックする

    //GPUにコマンドリストの実行を行わせる
    ID3D12CommandList* commandLists[] = { dxCommon_->GetCommandList() };
    dxCommon_->GetCommandQueue()->ExecuteCommandLists(1, commandLists);
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
    hr = dxCommon_->GetCommandList()->Reset(dxCommon_->GetCommandAllocator(), nullptr);
    assert(SUCCEEDED(hr));

}

void DrawManager::SetFrameData(const CameraForGPU& camera, const DirectionalLight& light, const PointLight& pointLight, const SpotLight& spotLight) {
    if (cameraData_) {
        *cameraData_ = camera;
    }
    if (directionalLightData_) {
        *directionalLightData_ = light;
    }
    if (pointLightData_) {
        *pointLightData_ = pointLight;
    }
    if (spotLightData_) {
        *spotLightData_ = spotLight;
    }
}

void DrawManager::DrawTriangle(
    TriangleClass * triangle
) {

    /*三角形を表示しよう*/
    //RootSignatureを設定。PSOに設定しているけど別途指定が必要
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &triangle->GetD3D12Resource()->vertexBufferView_); // VBVを設定
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

    /*三角形の色を変えよう*/

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, triangle->GetD3D12Resource()->materialResource_->GetGPUVirtualAddress());

    /*三角形を動かそう*/

    //wvp用のCbufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, triangle->GetD3D12Resource()->transformationResource_->GetGPUVirtualAddress());

    // ↓ ここから追加
    // GSパイプライン用のwvp CBufferをルートパラメータ[8]に設定
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(8, triangle->GetD3D12Resource()->transformationResource_->GetGPUVirtualAddress());
    // ↑ ここまで追加

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, triangle->GetD3D12Resource()->textureHandle_);

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    dxCommon_->GetCommandList()->DrawInstanced(static_cast<UINT>(triangle->GetD3D12Resource()->vertexDataList_.size()), 1, 0, 0);

}

void DrawManager::DrawSprite(Sprite* sprite) {

    // 2. パイプラインの基本構成（RootSignature, PSO）

    //RootSignatureを設定。PSOに設定しているけど別途指定が必要
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());

    // 3. バッファ設定（VBV、IBV、Topology）

    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //VBVを設定
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &sprite->GetD3D12Resource()->vertexBufferView_);
    //IBVを設定
    dxCommon_->GetCommandList()->IASetIndexBuffer(&sprite->GetD3D12Resource()->indexBufferView_);

    // 4. 定数バッファ（CBV）やライト用CBVの設定

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, sprite->GetD3D12Resource()->materialResource_->GetGPUVirtualAddress());

    //wvp用のCbufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, sprite->GetD3D12Resource()->transformationResource_->GetGPUVirtualAddress());

    // 5. テクスチャ用のDescriptor Table設定（SRV）

    /*テクスチャを貼ろう*/

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, sprite->GetD3D12Resource()->textureHandle_);

    // 6. 描画

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    dxCommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>(sprite->GetD3D12Resource()->indexDataList_.size()), 1, 0, 0, 0);

}

void DrawManager::DrawSphere(SphereClass* sphere) {

    /*三角形を表示しよう*/
    //RootSignatureを設定。PSOに設定しているけど別途指定が必要
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &sphere->GetD3D12Resource()->vertexBufferView_); // VBVを設定
    //IBVを設定
    dxCommon_->GetCommandList()->IASetIndexBuffer(&sphere->GetD3D12Resource()->indexBufferView_);
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    /*三角形の色を変えよう*/

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, sphere->GetD3D12Resource()->materialResource_->GetGPUVirtualAddress());

    /*三角形を動かそう*/

    //wvp用のCbufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, sphere->GetD3D12Resource()->transformationResource_->GetGPUVirtualAddress());

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, sphere->GetD3D12Resource()->textureHandle_);

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    dxCommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>(sphere->GetD3D12Resource()->indexDataList_.size()), 1, 0, 0, 0);

}

void DrawManager::DrawCylinder(CylinderClass* cylinder) {

    // RootSignature / IA / VB/IB 設定（省略せずそのまま）
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &cylinder->GetD3D12Resource()->vertexBufferView_);
    dxCommon_->GetCommandList()->IASetIndexBuffer(&cylinder->GetD3D12Resource()->indexBufferView_);
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // CBV / SRV
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, cylinder->GetD3D12Resource()->materialResource_->GetGPUVirtualAddress());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, cylinder->GetD3D12Resource()->transformationResource_->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, cylinder->GetD3D12Resource()->textureHandle_);

    // Draw
    dxCommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>(cylinder->GetD3D12Resource()->indexDataList_.size()), 1, 0, 0, 0);
}

void DrawManager::DrawParticle(ParticleSystem* resource) {

    // インスタンス数が0の場合は描画しない
    if (resource->GetInstanceCount() == 0) {
        return;
    }

    // RootSignature を設定（PSO とは別にコマンドリスト上で設定が必要）
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());

    // IA 設定: VB/IB/Topology
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &resource->GetD3D12Resource()->vertexBufferView_);
    dxCommon_->GetCommandList()->IASetIndexBuffer(&resource->GetD3D12Resource()->indexBufferView_);
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- CBV のバインド ---
    // 0: 既存のマテリアル CBV（互換性維持のために常にバインド）
    //    (rootParameters[0] に対応、PixelShader 側の b0 想定)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, resource->GetD3D12Resource()->materialResource_->GetGPUVirtualAddress());

    // Particle 専用マテリアル CBV を root index 9 にバインド
    // - DirectXCommon.cpp の RootSignature で rootParameters[9] を ParticleMaterial (PS b5) に
    //   マップしているため、Draw 側はルート配列インデックス 9 を使って渡す必要があります。
    // - ここで渡すのは resource->GetD3D12Resource()->materialResource_->GetGPUVirtualAddress()
    //   （D3D12ResourceUtilParticle::materialResource_ が ParticleMaterial 構造体を保持している想定）
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(9, resource->GetD3D12Resource()->materialResource_->GetGPUVirtualAddress());

    // インスタンス用 SRV (VS 側で参照するインスタンス配列)
    auto instancing = resource->GetInstancingSrvHandleGPU();
    assert(instancing.ptr != 0 && "Instancing SRV handle is null or invalid");
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(4, resource->GetInstancingSrvHandleGPU());

    // テクスチャ (PS t0)
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, resource->GetD3D12Resource()->textureHandle_);

    // 描画コール: インデックス数 × インスタンス数
    dxCommon_->GetCommandList()->DrawIndexedInstanced(
        static_cast<UINT>(resource->GetD3D12Resource()->indexDataList_.size()),
        resource->GetInstanceCount(),
        0, 0, 0
    );
}

void DrawManager::DrawRegion(Region* region) {
    if (!region) { return; }
    const GpuMesh* gpuMesh = region->GetGpuMesh();
    if (!gpuMesh || gpuMesh->vertexCount == 0 || region->GetInstanceCount() == 0) { return; }

    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());

    // IA設定 (共有リソースから)
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &gpuMesh->vertexBufferView);
    if (gpuMesh->indexCount > 0) {
        dxCommon_->GetCommandList()->IASetIndexBuffer(&gpuMesh->indexBufferView);
    }

    // CBV/SRV設定 (インスタンスリソースから)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, region->GetMaterialResource()->GetGPUVirtualAddress());
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, region->GetTextureHandle());
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(4, region->GetInstancingSrvHandleGPU());

    // 描画
    if (gpuMesh->indexCount > 0) {
        dxCommon_->GetCommandList()->DrawIndexedInstanced(gpuMesh->indexCount, region->GetInstanceCount(), 0, 0, 0);
    } else {
        dxCommon_->GetCommandList()->DrawInstanced(gpuMesh->vertexCount, region->GetInstanceCount(), 0, 0);
    }
}

void DrawManager::DrawSphereRegion(SphereRegion* region) {
    if (!region) { return; }
    if (region->GetIndexCount() == 0 || region->GetInstanceCount() == 0) { return; }

    // RootSignature
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());

    // IA
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &region->GetVertexBufferView());
    dxCommon_->GetCommandList()->IASetIndexBuffer(&region->GetIndexBufferView());

    // CBV (PS)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, region->GetMaterialResource()->GetGPUVirtualAddress());          // PS b0

    // SRV (PS t0 / VS t0)
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, region->GetTextureHandle());            // PS t0
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(4, region->GetInstancingSrvHandleGPU());   // VS t0

    // Draw
    dxCommon_->GetCommandList()->DrawIndexedInstanced(region->GetIndexCount(), region->GetInstanceCount(), 0, 0, 0);
}

void DrawManager::DrawTetraRegion(TetraRegion* region) {
    if (!region) return;
    if (region->GetIndexCount() == 0 || region->GetInstanceCount() == 0) return;

    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());

    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &region->GetVertexBufferView());
    dxCommon_->GetCommandList()->IASetIndexBuffer(&region->GetIndexBufferView());

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, region->GetMaterialResource()->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, region->GetTextureHandle());
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(4, region->GetInstancingSrvHandleGPU());

    dxCommon_->GetCommandList()->DrawIndexedInstanced(region->GetIndexCount(), region->GetInstanceCount(), 0, 0, 0);
}

void DrawManager::DrawByIndex(D3D12ResourceUtil* resource) {

    /*三角形を表示しよう*/
    //RootSignatureを設定。PSOに設定しているけど別途指定が必要
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &resource->vertexBufferView_); // VBVを設定
    //IBVを設定
    dxCommon_->GetCommandList()->IASetIndexBuffer(&resource->indexBufferView_);
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    /*三角形の色を変えよう*/

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, resource->materialResource_->GetGPUVirtualAddress());

    /*三角形を動かそう*/

    //wvp用のCbufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, resource->transformationResource_->GetGPUVirtualAddress());

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, resource->textureHandle_);

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    dxCommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>(resource->indexDataList_.size()), 1, 0, 0, 0);

}

void DrawManager::DrawByVertex(D3D12ResourceUtil* resource) {

    /*三角形を表示しよう*/
    //RootSignatureを設定。PSOに設定しているけど別途指定が必要
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &resource->vertexBufferView_); // VBVを設定
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    /*三角形の色を変えよう*/

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, resource->materialResource_->GetGPUVirtualAddress());

    /*三角形を動かそう*/

    //wvp用のCbufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, resource->transformationResource_->GetGPUVirtualAddress());

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, resource->textureHandle_);

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    dxCommon_->GetCommandList()->DrawInstanced(static_cast<UINT>(resource->vertexDataList_.size()), 1, 0, 0);

}

void DrawManager::DrawLine2D(Line2DClass* line) {
    D3D12ResourceUtilLine* resource = line->GetD3D12Resource();

    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &resource->vertexBufferView_);
    dxCommon_->GetCommandList()->IASetIndexBuffer(&resource->indexBufferView_);

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, resource->transformationResource_->GetGPUVirtualAddress());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, resource->materialResource_->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->DrawIndexedInstanced(2, 1, 0, 0, 0);

}

void DrawManager::DrawLine3D(Line3DClass* line) {
    D3D12ResourceUtilLine* resource = line->GetD3D12Resource();

    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &resource->vertexBufferView_);
    dxCommon_->GetCommandList()->IASetIndexBuffer(&resource->indexBufferView_);

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, resource->transformationResource_->GetGPUVirtualAddress());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, resource->materialResource_->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->DrawIndexedInstanced(2, 1, 0, 0, 0);
}

void DrawManager::DrawSpriteRegion(SpriteRegion* region) {
    if (!region) return;
    auto* res = region->GetSpriteResource();
    if (!res) return;
    const UINT idxCount = region->GetIndexCount();
    const UINT instCount = region->GetInstanceCountU32();
    if (idxCount == 0 || instCount == 0) return;

    // RootSignature
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());

    // IA
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &res->vertexBufferView_);
    dxCommon_->GetCommandList()->IASetIndexBuffer(&res->indexBufferView_);

    // CBV (PS)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, res->materialResource_->GetGPUVirtualAddress());          // PS b0

    // SRV (PS t0 / VS t0)
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, res->textureHandle_);              // PS t0 (テクスチャ)
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(4, region->GetInstancingSrvHandleGPU()); // VS t0 (インスタンス)

    // Draw
    dxCommon_->GetCommandList()->DrawIndexedInstanced(idxCount, instCount, 0, 0, 0);
}

void DrawManager::DrawSharedMesh(const GpuMesh* gpuMesh, D3D12ResourceUtil* instanceResource) {
    if (!gpuMesh || !instanceResource) return;

    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    
    // IA設定 (共有リソースから)
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &gpuMesh->vertexBufferView);
    if (gpuMesh->indexCount > 0) {
        dxCommon_->GetCommandList()->IASetIndexBuffer(&gpuMesh->indexBufferView);
    }

    // CBV/SRV設定 (インスタンスリソースから)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, instanceResource->materialResource_->GetGPUVirtualAddress());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, instanceResource->transformationResource_->GetGPUVirtualAddress());
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, instanceResource->textureHandle_);

    // 描画
    if (gpuMesh->indexCount > 0) {
        dxCommon_->GetCommandList()->DrawIndexedInstanced(gpuMesh->indexCount, 1, 0, 0, 0);
    } else {
        dxCommon_->GetCommandList()->DrawInstanced(gpuMesh->vertexCount, 1, 0, 0);
    }
}

void DrawManager::DrawCube(CubeClass* cube) {

    /*三角形を表示しよう*/
    //RootSignatureを設定。PSOに設定しているけど別途指定が必要
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &cube->GetD3D12Resource()->vertexBufferView_); // VBVを設定
    //IBVを設定
    dxCommon_->GetCommandList()->IASetIndexBuffer(&cube->GetD3D12Resource()->indexBufferView_);
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    /*三角形の色を変えよう*/

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, cube->GetD3D12Resource()->materialResource_->GetGPUVirtualAddress());

    /*三角形を動かそう*/

    //wvp用のCbufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, cube->GetD3D12Resource()->transformationResource_->GetGPUVirtualAddress());

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, cube->GetD3D12Resource()->textureHandle_);

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    dxCommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>(cube->GetD3D12Resource()->indexDataList_.size()), 1, 0, 0, 0);
}

void DrawManager::DrawModel(const ManagedModel* model, const TransformationMatrix& matrix) {
    if (!model || !model->cpuModel || !dxCommon_) return;

    // オブジェクトごとの変換行列用リソースを一時的に確保
    Microsoft::WRL::ComPtr<ID3D12Resource> transformResource = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
    TransformationMatrix* transformData = nullptr;
    transformResource->Map(0, nullptr, reinterpret_cast<void**>(&transformData));
    *transformData = matrix;
    D3D12_GPU_VIRTUAL_ADDRESS transformVA = transformResource->GetGPUVirtualAddress();

    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // モデル内の全メッシュをループして描画
    for (size_t i = 0; i < model->gpuMeshes.size(); ++i) {
        const auto& gpuMesh = model->gpuMeshes[i];
        const auto& gpuMaterial = model->gpuMaterials[i];

        if (!gpuMesh || !gpuMaterial) continue;

        // IA (頂点/インデックス)
        dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &gpuMesh->vertexBufferView);
        if (gpuMesh->indexCount > 0) {
            dxCommon_->GetCommandList()->IASetIndexBuffer(&gpuMesh->indexBufferView);
        }

        // CBV (マテリアル/Transform)
        dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, gpuMaterial->materialResource->GetGPUVirtualAddress());
        dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformVA);

        // SRV (テクスチャ)
        dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, gpuMaterial->textureHandle);

        // 描画コマンド
        if (gpuMesh->indexCount > 0) {
            dxCommon_->GetCommandList()->DrawIndexedInstanced(gpuMesh->indexCount, 1, 0, 0, 0);
        } else {
            dxCommon_->GetCommandList()->DrawInstanced(gpuMesh->vertexCount, 1, 0, 0);
        }
    }
    // 一時リソースはフレーム終了後に解放される
}