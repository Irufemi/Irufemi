#include "DrawManager.h"

#include<Windows.h>
#include <cassert>

#include <dxgidebug.h>
#include "3D/SphereClass.h"
#include "2D/Sprite.h"
#include "2D/SpriteRegion.h"
#include "3D/ObjClass.h"
#include "3D/TriangleClass.h"
#include "3D/ParticleClass.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "3D/CylinderClass.h"
#include "3D/Region.h"
#include "3D/SphereRegion.h"
#include "3D/TetraRegion.h" // 追加インクルード

#include "source/D3D12ResourceUtil.h"
#include "engine/directX/DirectXCommon.h"



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

void DrawManager::Finalize() {
    // 静的フォールバックCBVを解放（デバイス参照を外す）
    if (gNullPointLight) {
        // マップ済みでもUnmap不要だが、気になるなら解除
        // gNullPointLight->Unmap(0, nullptr);
        gNullPointLight.Reset();
        gNullPointLightVA = 0;
    }
    if (gNullSpotLight) {
        // gNullSpotLight->Unmap(0, nullptr);
        gNullSpotLight.Reset();
        gNullSpotLightVA = 0;
    }
    pointLight_ = nullptr;
    spotLight_ = nullptr;
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

    /*開発のUIを出そう*/

    ///ImGuiを描画する

    //描画用のDescriptorHeapの設定
    ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
    dxCommon_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);
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

void DrawManager::EnsurePointLightResource() {
    if (!pointLight_) return;
    if (!pointLight_->GetResource()) {
        pointLight_->Initialize();
    }
}

void DrawManager::EnsureSpotLightResource() {
    if (!spotLight_) return;
    if (!spotLight_->GetResource()) {
        spotLight_->Initialize();
    }

}

void DrawManager::DrawTriangle(
    D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
    ID3D12Resource* materialResource,
    ID3D12Resource* wvpResource,
    ID3D12Resource* directionalLightResource,
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU
) {

    /*三角形を表示しよう*/
    //RootSignatureを設定。PSOに設定しているけど別途指定が必要
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    /*三角形の色を変えよう*/

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

    /*三角形を動かそう*/

    //wvp用のCbufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());


    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, GetPointLightVA());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(7, GetSpotLightVA());

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    dxCommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);

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

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, sprite->GetD3D12Resource()->directionalLightResource_->GetGPUVirtualAddress());

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

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, sphere->GetD3D12Resource()->directionalLightResource_->GetGPUVirtualAddress());

    /*PhongReflectionModel*/

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, sphere->GetD3D12Resource()->cameraResource_->GetGPUVirtualAddress());

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, sphere->GetD3D12Resource()->textureHandle_);

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, GetPointLightVA());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(7, GetSpotLightVA());

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
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, cylinder->GetD3D12Resource()->directionalLightResource_->GetGPUVirtualAddress());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, cylinder->GetD3D12Resource()->cameraResource_->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, cylinder->GetD3D12Resource()->textureHandle_);

    // ←ここを直接メンバ参照から安全な VA 取得に変更
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, GetPointLightVA());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(7, GetSpotLightVA());

    // Draw
    dxCommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>(cylinder->GetD3D12Resource()->indexDataList_.size()), 1, 0, 0, 0);
}

void DrawManager::DrawParticle(ParticleClass* resource) {

    /*三角形を表示しよう*/
    //RootSignatureを設定。PSOに設定しているけど別途指定が必要
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &resource->GetD3D12Resource()->vertexBufferView_); // VBVを設定
    //IBVを設定
    dxCommon_->GetCommandList()->IASetIndexBuffer(&resource->GetD3D12Resource()->indexBufferView_);
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    /*三角形の色を変えよう*/

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, resource->GetD3D12Resource()->materialResource_->GetGPUVirtualAddress());

    auto instancing = resource->GetInstancingSrvHandleGPU();
    assert(instancing.ptr != 0 && "Instancing SRV handle is null or invalid");
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(4, resource->GetInstancingSrvHandleGPU());

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, resource->GetD3D12Resource()->textureHandle_);

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    dxCommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>(resource->GetD3D12Resource()->indexDataList_.size()), resource->GetInstanceCount(), 0, 0, 0);

}

void DrawManager::DrawRegion(Region* region) {
    if (!region) { return; }
    if (region->GetVertexCount() == 0 || region->GetInstanceCount() == 0) { return; }

    // RootSignature
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetRootSignature());

    // IA
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &region->GetVertexBufferView());

    // CBV (PS)
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, region->GetMaterialResource()->GetGPUVirtualAddress());          // PS b0
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, region->GetDirectionalLightResource()->GetGPUVirtualAddress());  // PS b1
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, region->GetCameraResource()->GetGPUVirtualAddress());            // PS b2

    // SRV (PS t0 / VS t0)
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, region->GetTextureHandle());         // PS t0
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(4, region->GetInstancingSrvHandleGPU()); // VS t0

    // オプション：ポイント/スポットライト（他描画と統一）
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, GetPointLightVA());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(7, GetSpotLightVA());

    // Draw
    dxCommon_->GetCommandList()->DrawInstanced(region->GetVertexCount(), region->GetInstanceCount(), 0, 0);
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
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, region->GetDirectionalLightResource()->GetGPUVirtualAddress());  // PS b1
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, region->GetCameraResource()->GetGPUVirtualAddress());            // PS b2

    // SRV (PS t0 / VS t0)
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, region->GetTextureHandle());            // PS t0
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(4, region->GetInstancingSrvHandleGPU());   // VS t0

    // ライト（フォールバック込み）
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, GetPointLightVA());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(7, GetSpotLightVA());

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
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, region->GetDirectionalLightResource()->GetGPUVirtualAddress());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, region->GetCameraResource()->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, region->GetTextureHandle());
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(4, region->GetInstancingSrvHandleGPU());

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, GetPointLightVA());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(7, GetSpotLightVA());

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

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, resource->directionalLightResource_->GetGPUVirtualAddress());

    /*PhongReflectionModel*/

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, resource->cameraResource_->GetGPUVirtualAddress());

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, resource->textureHandle_);

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, GetPointLightVA());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(7, GetSpotLightVA());

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

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, resource->directionalLightResource_->GetGPUVirtualAddress());

    /*PhongReflectionModel*/

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, resource->cameraResource_->GetGPUVirtualAddress());

    /*テクスチャを貼ろう*/

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, resource->textureHandle_);

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, GetPointLightVA());
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(7, GetSpotLightVA());

    /*三角形を表示しよう*/

    //描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
    dxCommon_->GetCommandList()->DrawInstanced(static_cast<UINT>(resource->vertexDataList_.size()), 1, 0, 0);

}

// SetPointLight / SetSpotLight はクラス保持のデータ差し替え用（任意）
void DrawManager::SetPointLight(PointLight& info) {
    if (!pointLight_) return;
    pointLight_->SetData(&info);
}
void DrawManager::SetSpotLight(SpotLight& info) {
    if (!spotLight_) return;
    spotLight_->SetData(&info);
}

D3D12_GPU_VIRTUAL_ADDRESS DrawManager::GetPointLightVA() {
    EnsureNullPointLight(dxCommon_);
    EnsurePointLightResource();
    return (pointLight_ && pointLight_->GetResource())
        ? pointLight_->GetResource()->GetGPUVirtualAddress()
        : gNullPointLightVA;
}

D3D12_GPU_VIRTUAL_ADDRESS DrawManager::GetSpotLightVA() {
    EnsureNullSpotLight(dxCommon_);
    EnsureSpotLightResource();
    return (spotLight_ && spotLight_->GetResource())
        ? spotLight_->GetResource()->GetGPUVirtualAddress()
        : gNullSpotLightVA;
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
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, res->directionalLightResource_->GetGPUVirtualAddress());  // PS b1
    // 2Dでは camera CBV 未使用のため省略（必要なら SetGraphicsRootConstantBufferView(5, ...)）

    // SRV (PS t0 / VS t0)
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, res->textureHandle_);              // PS t0 (テクスチャ)
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(4, region->GetInstancingSrvHandleGPU()); // VS t0 (インスタンス)

    // Draw
    dxCommon_->GetCommandList()->DrawIndexedInstanced(idxCount, instCount, 0, 0, 0);
}