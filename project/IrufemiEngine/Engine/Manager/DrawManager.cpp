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
#include "../Graphics/DirectX/DirectXCommon.h"
#include "../../Resource/Model/ModelManager.h"
#include "../../engine/IrufemiEngine.h"
#include "../Graphics/Data/CameraForGPU.h"
#include "../Graphics/Data/DirectionalLight.h"
#include "../Graphics/Data/PointLight.h"
#include "../Graphics/Data/SpotLight.h"
#include "../Graphics/Data/AreaLight.h"
#include "../Core/Math/Math.h"
#include "../../Resource/Model/Data/SkinCluster.h"
#include "../Graphics/DirectX/ShadowMap.h"
#include "../../Resource/Texture/TextureManager.h"


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

DrawManager::DrawManager() {}
DrawManager::~DrawManager() {}

void DrawManager::Initialize(DirectXCommon* dx) {
    dxCommon_ = dx;
    commandList_ = dx->GetCommandList();

    // 定数バッファのサイズ (256バイトアラインメント)
    const size_t cameraSize = (sizeof(CameraForGPU) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    const size_t lightCommonSize = (sizeof(LightCommonData) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    const uint32_t kMaxLights = 1024;

    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        auto& fr = frameResources_[i];

        // フレーム定数バッファ (Camera + LightCommonData)
        fr.frameResource = dxCommon_->CreateBufferResource(cameraSize + lightCommonSize);
        uint8_t* mapped = nullptr;
        fr.frameResource->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

        fr.cameraData = reinterpret_cast<CameraForGPU*>(mapped);
        fr.lightCommonData = reinterpret_cast<LightCommonData*>(mapped + cameraSize);

        fr.frameData.camera = fr.frameResource->GetGPUVirtualAddress();
        fr.frameData.lightCommon = fr.frameData.camera + cameraSize;

        // StructuredBuffer の初期化 (ひとまず1024個分を確保)
        fr.pointLightResource = dxCommon_->CreateBufferResource(sizeof(PointLight) * kMaxLights);
        fr.spotLightResource = dxCommon_->CreateBufferResource(sizeof(SpotLight) * kMaxLights);
        fr.areaLightResource = dxCommon_->CreateBufferResource(sizeof(AreaLight) * kMaxLights);

        // ライト SRV デスクリプタの一括確保 (Point, Spot, Area 用に 3 つ)
        auto pool = dxCommon_->GetSrvPool();
        fr.lightSrvBaseIndex = pool->Allocate(3);
        fr.lightSrvHandle = pool->GetGPUHandle(fr.lightSrvBaseIndex);

        // StructuredBuffer SRV の作成
        pool->CreateSRVForStructuredBuffer(fr.lightSrvBaseIndex + 0, fr.pointLightResource.Get(), kMaxLights, sizeof(PointLight));
        pool->CreateSRVForStructuredBuffer(fr.lightSrvBaseIndex + 1, fr.spotLightResource.Get(), kMaxLights, sizeof(SpotLight));
        pool->CreateSRVForStructuredBuffer(fr.lightSrvBaseIndex + 2, fr.areaLightResource.Get(), kMaxLights, sizeof(AreaLight));
    }

    // シャドウマップの初期化 (2048x2048) - 全フレーム分
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        shadowMaps_[i] = std::make_unique<ShadowMap>();
        shadowMaps_[i]->Initialize(dxCommon_, 2048, 2048);
    }
}

void DrawManager::ExecuteComputePasses() {
    for (auto* task : computeTasks_) {
        task->DispatchCompute();
    }
    computeTasks_.clear();
}

void DrawManager::Finalize() {
    auto* srvPool = dxCommon_->GetSrvPool();
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        auto& fr = frameResources_[i];
        if (fr.frameResource) {
            fr.frameResource->Unmap(0, nullptr);
            fr.frameResource.Reset();
        }
        fr.pointLightResource.Reset();
        fr.spotLightResource.Reset();
        fr.areaLightResource.Reset();

        // SRVの解放
        if (srvPool && fr.lightSrvBaseIndex != 0xFFFFFFFFu) {
            for (uint32_t j = 0; j < 3; ++j) {
                srvPool->FreeAfterFence(fr.lightSrvBaseIndex + j, dxCommon_->GetFenceValue(i));
            }
            fr.lightSrvBaseIndex = 0xFFFFFFFFu;
        }
    }

    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        shadowMaps_[i].reset();
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

    // 1. GPU同期 (これから使うスロットが前回の使用（通し番号）を終えるまで待つ)
    ID3D12Fence* fence = dxCommon_->GetFence();
    uint64_t waitValue = dxCommon_->GetFenceValue(); // このスロットが最後に使われた時の通し番号
    if (fence->GetCompletedValue() < waitValue) {
        fence->SetEventOnCompletion(waitValue, dxCommon_->GetFenceEvent());
        WaitForSingleObject(dxCommon_->GetFenceEvent(), INFINITE);
    }

    // 2. コマンドリストとアロケータのリセット (現在のフレーム用)
    ID3D12CommandAllocator* allocator = dxCommon_->GetCommandAllocator();
    HRESULT hr = allocator->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList_->Reset(allocator, nullptr);
    assert(SUCCEEDED(hr));

    // フレーム開始時に、ポーズ中でSetFrameDataが呼ばれなくてもバッファが常に同期待ちにならないようキャッシュを現在のバッファへコピーする
    SyncCachedFrameData();

    // バックバッファとRTV/DSVの取得 (これはスワップチェーン依存なのでそのままでよい)

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

    // レンダーターゲット追跡の更新 (バックバッファ)
    currentRenderTexture_ = nullptr;

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
    } else if (textureManager_) {
        commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::EnvMap, textureManager_->GetWhiteCubeMapHandle());
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


    // 1. フェンスをシグナル (通し番号をインクリメントして記録)
    uint64_t nextValue = dxCommon_->IncrementGlobalFence();
    dxCommon_->GetFenceValue() = nextValue; // このスロットの完了番号として保存
    dxCommon_->GetCommandQueue()->Signal(dxCommon_->GetFence(), nextValue);
 
    // 2. 次のフレームへインデックスを進める
    dxCommon_->AdvanceFrameIndex();

    dxCommon_->UpdateFixFPS();
}

void DrawManager::SetFrameData(const CameraForGPU& camera, const DirectionalLight& light, const std::vector<PointLight*>& pointLights, const std::vector<SpotLight*>& spotLights, const std::vector<AreaLight*>& areaLights) {
    cachedCamera_ = camera;
    cachedDirectionalLight_ = light;
    
    cachedPointLights_.clear();
    for (auto* pl : pointLights) cachedPointLights_.push_back(*pl);
    
    cachedSpotLights_.clear();
    for (auto* sl : spotLights) cachedSpotLights_.push_back(*sl);
    
    cachedAreaLights_.clear();
    for (auto* al : areaLights) cachedAreaLights_.push_back(*al);
    
    SyncCachedFrameData();
}

void DrawManager::SyncCachedFrameData() {
    auto& fr = frameResources_[dxCommon_->GetFrameIndex()];

    if (fr.cameraData) { *fr.cameraData = cachedCamera_; }
    if (fr.lightCommonData) {
        // ライト共通データの更新（b1）
        fr.lightCommonData->directionalLight = cachedDirectionalLight_;
        fr.lightCommonData->pointLightCount = static_cast<int32_t>(cachedPointLights_.size());
        fr.lightCommonData->spotLightCount = static_cast<int32_t>(cachedSpotLights_.size());
        fr.lightCommonData->areaLightCount = static_cast<int32_t>(cachedAreaLights_.size());

        // シャドウマップの行列更新
        ShadowMap* shadowMap = shadowMaps_[dxCommon_->GetFrameIndex()].get();
        if (shadowMap) {
            // カメラの位置を注視点として追従させる
            shadowMap->UpdateMatrix(cachedDirectionalLight_.direction, cachedCamera_.worldPosition, 128.0f);
            fr.lightCommonData->viewProjection = shadowMap->GetViewProjection();
        }
    }

    // 各 StructuredBuffer へ書き込み
    auto copyLights = [](ID3D12Resource* res, const auto& lightVec) {
        if (!res || lightVec.empty()) return;
        using LightType = std::remove_pointer_t<typename std::decay_t<decltype(lightVec)>::value_type>;
        LightType* mapped = nullptr;
        res->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
        for (size_t i = 0; i < lightVec.size(); ++i) {
            mapped[i] = lightVec[i];
        }
        res->Unmap(0, nullptr);
    };

    copyLights(fr.pointLightResource.Get(), cachedPointLights_);
    copyLights(fr.spotLightResource.Get(), cachedSpotLights_);
    copyLights(fr.areaLightResource.Get(), cachedAreaLights_);
}

void DrawManager::SetEnvironmentMap(D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle) {
    environmentMapHandle_ = envMapHandle;
}



void DrawManager::SubmitSprite(const Object2DResource* resource) {
    if (!resource) return;
    SpritePacket p{};
    p.resource = resource;
    p.blendMode = dxCommon_->GetEngine()->currentBlend_;
    p.depthWrite = dxCommon_->GetEngine()->currentDepth_;
    p.cullMode = dxCommon_->GetEngine()->currentCull_;
    spriteQueue_.push_back(p);
}

void DrawManager::DrawSprite(const SpritePacket& packet) {
    const Object2DResource* resource = packet.resource;
    if (!resource || !commandList_) return;

    // トポロジ設定
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 頂点バッファとインデックスバッファの設定
    commandList_->IASetVertexBuffers(0, 1, &resource->vertexBufferView_);
    commandList_->IASetIndexBuffer(&resource->indexBufferView_);

    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, resource->GetMaterialVAddress());
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Transform, resource->GetTransformVAddress());
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, resource->textureHandle_);

    commandList_->DrawIndexedInstanced(resource->indexCount_, 1, 0, 0, 0);
}

void DrawManager::SubmitParticle(const ParticleResource* resource, uint32_t instanceCount) {
    if (!resource || instanceCount == 0) return;
    ParticlePacket p{};
    p.resource = resource;
    p.instanceCount = instanceCount;
    p.blendMode = dxCommon_->GetEngine()->currentBlend_;
    p.depthWrite = dxCommon_->GetEngine()->currentDepth_;
    p.cullMode = dxCommon_->GetEngine()->currentCull_;
    particleQueue_.push_back(p);
}

void DrawManager::DrawParticle(const ParticlePacket& packet) {
    const ParticleResource* resource = packet.resource;
    if (!resource || !commandList_ || packet.instanceCount == 0) return;

    // IA 設定: VB/IB/Topology
    commandList_->IASetVertexBuffers(0, 1, &resource->vertexBufferView_);
    commandList_->IASetIndexBuffer(&resource->indexBufferView_);
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- CBV のバインド ---
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, resource->GetMaterialVAddress());

    // インスタンス用 SRV (VS 側で参照するインスタンス配列)
    uint32_t frameIndex = dxCommon_->GetFrameIndex();
    assert(resource->instancingSrvHandleGPU_[frameIndex].ptr != 0 && "Instancing SRV handle is null or invalid");
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Instancing, resource->instancingSrvHandleGPU_[frameIndex]);

    // テクスチャ (PS t0)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, resource->textureHandle_);

    // 描画コール: インデックス数 × インスタンス数
    commandList_->DrawIndexedInstanced(
        resource->indexCount_,
        packet.instanceCount,
        0, 0, 0
    );
}

void DrawManager::SubmitModelRegion(const ModelRegionPacket& packet) {
    modelRegionQueue_.push_back(packet);
}

void DrawManager::DrawModelRegion(const ModelRegionPacket& packet) {
    const GpuMesh* gpuMesh = packet.gpuMesh;
    if (!gpuMesh || gpuMesh->vertexCount == 0 || packet.instanceCount == 0) { return; }

    // IA設定 (共有リソースから)
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList_->IASetVertexBuffers(0, 1, &gpuMesh->vertexBufferView);
    if (gpuMesh->indexCount > 0) {
        commandList_->IASetIndexBuffer(&gpuMesh->indexBufferView);
    }

    // Material (CBV)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, packet.materialAddress);

    // Texture (SRV)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, packet.textureHandle);

    // Instances (SRV)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Instancing, packet.instancingSrvHandleGPU);

    // Draw
    if (gpuMesh->indexCount > 0) {
        commandList_->DrawIndexedInstanced(gpuMesh->indexCount, packet.instanceCount, 0, 0, 0);
    } else {
        commandList_->DrawInstanced(gpuMesh->vertexCount, packet.instanceCount, 0, 0);
    }
}

void DrawManager::SubmitRegion(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, D3D12_GPU_VIRTUAL_ADDRESS materialAddress, const D3D12_GPU_DESCRIPTOR_HANDLE& textureHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& instancingSrvHandleGPU, const UINT& indexCount, const UINT& instanceCount) {
    if (indexCount == 0 || instanceCount == 0) { return; }
    RegionPacket p{};
    p.vertexBufferView = vertexBufferView;
    p.indexBufferView = indexBufferView;
    p.materialAddress = materialAddress;
    p.textureHandle = textureHandle;
    p.instancingSrvHandleGPU = instancingSrvHandleGPU;
    p.indexCount = indexCount;
    p.instanceCount = instanceCount;
    p.blendMode = dxCommon_->GetEngine()->currentBlend_;
    p.depthWrite = dxCommon_->GetEngine()->currentDepth_;
    p.cullMode = dxCommon_->GetEngine()->currentCull_;
    regionQueue_.push_back(p);
}

void DrawManager::DrawRegion(const RegionPacket& packet) {
    if (packet.indexCount == 0 || packet.instanceCount == 0) { return; }

    // IA
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList_->IASetVertexBuffers(0, 1, &packet.vertexBufferView);
    commandList_->IASetIndexBuffer(&packet.indexBufferView);

    // CBV (PS)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, packet.materialAddress);          // PS b0

    // SRV (PS t0 / VS t0)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, packet.textureHandle);            // PS t0
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Instancing, packet.instancingSrvHandleGPU);   // VS t0

    // Draw
    commandList_->DrawIndexedInstanced(packet.indexCount, packet.instanceCount, 0, 0, 0);
}

void DrawManager::SubmitLineInstanced(const LineResource* resource, const D3D12_GPU_DESCRIPTOR_HANDLE& instancingSrvHandleGPU, const UINT& instanceCount) {
    if (!resource || instanceCount == 0) return;
    LinePacket p{};
    p.resource = resource;
    p.instancingSrvHandleGPU = instancingSrvHandleGPU;
    p.instanceCount = instanceCount;
    p.blendMode = dxCommon_->GetEngine()->currentBlend_;
    p.depthWrite = dxCommon_->GetEngine()->currentDepth_;
    p.cullMode = dxCommon_->GetEngine()->currentCull_;
    lineQueue_.push_back(p);
}

void DrawManager::DrawLineInstanced(const LinePacket& packet) {
    const LineResource* resource = packet.resource;
    if (!resource || packet.instanceCount == 0) return;

    // IA
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList_->IASetVertexBuffers(0, 1, &resource->vertexBufferView_);
    commandList_->IASetIndexBuffer(&resource->indexBufferView_);

    // SRV (VS t1)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::LineInstancing, packet.instancingSrvHandleGPU);

    // Draw
    commandList_->DrawIndexedInstanced(2, packet.instanceCount, 0, 0, 0);
}

void DrawManager::DispatchSkinning(const SkinCluster& skinCluster, const ManagedModel* model, uint32_t numVertices) {
    if (!model || !model->gpuMeshes[0] || !dxCommon_) return;

    // --- コンピュートシェーダーによるスキニング実行 ---
    // PSOをコンピュート用に切り替え
    commandList_->SetPipelineState(dxCommon_->GetSkinningComputePSO());

    // RootSignatureはSkipして共通のComputeRootSignatureを使用する想定
    // (PSO設定時にセットされているはずだが、念のため管理が必要な場合はここでセット)

    // Parameterの設定
    uint32_t frameIndex = dxCommon_->GetFrameIndex();
    
    // 0: Palette (t0)
    commandList_->SetComputeRootDescriptorTable(0, skinCluster.paletteSrvHandle[frameIndex].second);
    // 1: Input Vertices (t1) (最初のメッシュの頂点を使用)
    commandList_->SetComputeRootDescriptorTable(1, model->gpuMeshes[0]->vertexSrvHandle);
    // 2: Influences (t2)
    commandList_->SetComputeRootDescriptorTable(2, skinCluster.influenceSrvHandle.second);
    // 3: Output Vertices (u0)
    commandList_->SetComputeRootDescriptorTable(3, skinCluster.skinnedVertexUavHandle[frameIndex].second);
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

void DrawManager::SubmitSkybox(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, D3D12_GPU_VIRTUAL_ADDRESS materialAddress, D3D12_GPU_VIRTUAL_ADDRESS transformationAddress, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, const UINT& indexCount) {
    SkyboxPacket p{};
    p.vertexBufferView = vertexBufferView;
    p.indexBufferView = indexBufferView;
    p.materialAddress = materialAddress;
    p.transformationAddress = transformationAddress;
    p.textureHandle = textureHandle;
    p.indexCount = indexCount;
    skyboxQueue_.push_back(p);
}

void DrawManager::DrawSkybox(const SkyboxPacket& packet) {

    commandList_->IASetVertexBuffers(0, 1, &packet.vertexBufferView); // VBVを設定
    //IBVを設定
    commandList_->IASetIndexBuffer(&packet.indexBufferView);
    //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ///CBVを設定する

    //マテリアルCBufferの場所を設定(ここでの第一引数の0はRootParameter配列の0番目であり、registerの0ではない)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, packet.materialAddress);

    //wvp用のCBufferの場所を設定(今回はRootParameter[1]に対してCBVの設定を行っている)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Transform, packet.transformationAddress);

    ///DescriptorTableを設定する

    //SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, packet.textureHandle);

    //描画！（DrawCall/ドローコール）。3頂点で1つのインスタンス。
    commandList_->DrawIndexedInstanced(packet.indexCount, 1, 0, 0, 0);
}

void DrawManager::SubmitStandard3D(const Object3DResource* resource, const D3D12_VERTEX_BUFFER_VIEW* vertexBufferViewOverride) {
    if (!resource) return;
    Standard3DPacket p{};
    p.resource = resource;
    p.vertexBufferViewOverride = vertexBufferViewOverride;
    p.blendMode = dxCommon_->GetEngine()->currentBlend_;
    p.depthWrite = dxCommon_->GetEngine()->currentDepth_;
    p.cullMode = dxCommon_->GetEngine()->currentCull_;
    standard3DQueue_.push_back(p);
}

void DrawManager::SubmitUI3D(const Object3DResource* resource, const D3D12_VERTEX_BUFFER_VIEW* vertexBufferViewOverride) {
    if (!resource) return;
    Standard3DPacket p{};
    p.resource = resource;
    p.vertexBufferViewOverride = vertexBufferViewOverride;
    p.blendMode = dxCommon_->GetEngine()->currentBlend_;
    p.depthWrite = dxCommon_->GetEngine()->currentDepth_;
    p.cullMode = dxCommon_->GetEngine()->currentCull_;
    ui3DQueue_.push_back(p);
}

void DrawManager::DrawStandard3D(const Standard3DPacket& packet) {
    const Object3DResource* resource = packet.resource;
    if (!resource || !commandList_) return;
    
    // トポロジ設定
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 頂点バッファの設定 (オーバーライドがあれば優先)
    if (packet.vertexBufferViewOverride) {
        commandList_->IASetVertexBuffers(0, 1, packet.vertexBufferViewOverride);
    } else {
        commandList_->IASetVertexBuffers(0, 1, &resource->vertexBufferView_);
    }
    commandList_->IASetIndexBuffer(&resource->indexBufferView_);

    // 各種リソースのバインド
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, resource->GetMaterialVAddress());
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Transform, resource->GetTransformVAddress());
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, resource->textureHandle_);

    // 描画
    commandList_->DrawIndexedInstanced(resource->indexCount_, 1, 0, 0, 0);
}


void DrawManager::SubmitGPUParticle(
    const D3D12_VERTEX_BUFFER_VIEW& vbv,
    const D3D12_INDEX_BUFFER_VIEW& ibv,
    uint32_t indexCount,
    D3D12_GPU_VIRTUAL_ADDRESS materialAddress,
    D3D12_GPU_VIRTUAL_ADDRESS perViewAddress,
    D3D12_GPU_VIRTUAL_ADDRESS emitterAddress,
    D3D12_GPU_DESCRIPTOR_HANDLE particleSrvHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
    uint32_t instanceCount,
    ID3D12Resource* particleResource
    ) {
    if (instanceCount == 0) return;
    GPUParticlePacket p{};
    p.vbv = vbv;
    p.ibv = ibv;
    p.indexCount = indexCount;
    p.materialAddress = materialAddress;
    p.perViewAddress = perViewAddress;
    p.emitterAddress = emitterAddress;
    p.particleSrvHandle = particleSrvHandle;
    p.textureHandle = textureHandle;
    p.instanceCount = instanceCount;
    p.particleResource = particleResource;
    p.blendMode = dxCommon_->GetEngine()->currentBlend_;
    p.depthWrite = dxCommon_->GetEngine()->currentDepth_;
    p.cullMode = dxCommon_->GetEngine()->currentCull_;
    gpuParticleQueue_.push_back(p);
}

void DrawManager::DrawGPUParticle(const GPUParticlePacket& packet) {
    if (!commandList_) return;

    // リソースバリヤー: UAV -> ShaderResource (読み取り)
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = packet.particleResource;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    if (packet.particleResource) {
        commandList_->ResourceBarrier(1, &barrier);
    }

    // IA 設定: VB/Topology
    commandList_->IASetVertexBuffers(0, 1, &packet.vbv);
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- CBV のバインド ---
    // (rootParameters[(UINT)RootSlot::Material] に対応、PixelShader 側の b0 想定)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, packet.materialAddress);
    // (rootParameters[(UINT)RootSlot::Transform] に対応、VertexShader 側の b0 想定)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Transform, packet.perViewAddress);
    // エミッター設定 (RootSlot::Special -> register b6)
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Special, packet.emitterAddress);

    // --- SRVのバインド ---
    // テクスチャ (PS t0)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, packet.textureHandle);
    // パーティクルデータ (VS t0 -> Slot 5: Instancing)
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Instancing, packet.particleSrvHandle);

    if (packet.indexCount > 0) {
        commandList_->IASetIndexBuffer(&packet.ibv);
        commandList_->DrawIndexedInstanced(packet.indexCount, packet.instanceCount, 0, 0, 0);
    } else {
        // 従来のビルボード互換
        commandList_->DrawInstanced(6, packet.instanceCount, 0, 0);
    }

    // リソースバリヤー: ShaderResource -> UAV (次のフレームの計算用に戻す)
    if (packet.particleResource) {
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        commandList_->ResourceBarrier(1, &barrier);
    }
}

void DrawManager::SubmitVoxelParticle(
    uint32_t instanceCount,
    const D3D12_VERTEX_BUFFER_VIEW& vbv,
    const D3D12_INDEX_BUFFER_VIEW& ibv,
    uint32_t indexCount,
    D3D12_GPU_VIRTUAL_ADDRESS perViewAddress,
    D3D12_GPU_VIRTUAL_ADDRESS emitterAddress,
    D3D12_GPU_DESCRIPTOR_HANDLE particleDataHandle,
    ID3D12Resource* particleResource,
    ID3D12PipelineState* drawPSO
) {
    if (instanceCount == 0) return;
    VoxelParticlePacket p{};
    p.instanceCount = instanceCount;
    p.vbv = vbv;
    p.ibv = ibv;
    p.indexCount = indexCount;
    p.perViewAddress = perViewAddress;
    p.emitterAddress = emitterAddress;
    p.particleDataHandle = particleDataHandle;
    p.particleResource = particleResource;
    p.drawPSO = drawPSO;
    voxelParticleQueue_.push_back(p);
}

void DrawManager::DrawVoxelParticle(const VoxelParticlePacket& packet) {
    if (!commandList_) return;

    // リソースバリヤー: UAV -> ShaderResource (読み取り)
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = packet.particleResource;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    if (packet.particleResource) {
        commandList_->ResourceBarrier(1, &barrier);
    }

    // VoxelParticle 専用PSOをバインド
    if (packet.drawPSO) {
        commandList_->SetPipelineState(packet.drawPSO);
    }

    // トポロジ設定
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 頂点バッファとインデックスバッファの設定
    commandList_->IASetVertexBuffers(0, 1, &packet.vbv);
    commandList_->IASetIndexBuffer(&packet.ibv);

    // VoxelParticle 特有のバインド
    // Slot 1: Transform (b0) <- Emitter
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Transform, packet.emitterAddress);
    // Slot 7: Special (b6) <- PerView
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Special, packet.perViewAddress);
    // Slot 9: LineInstancing (t1) <- ParticleData
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::LineInstancing, packet.particleDataHandle);

    commandList_->DrawIndexedInstanced(packet.indexCount, packet.instanceCount, 0, 0, 0);

    // リソースバリヤー: ShaderResource -> UAV (次のフレームの計算用に戻す)
    if (packet.particleResource) {
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        commandList_->ResourceBarrier(1, &barrier);
    }
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

    // レンダーターゲットを追跡
    currentRenderTexture_ = rt;

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

    // レンダーターゲット追跡のリセット
    currentRenderTexture_ = nullptr;
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

    auto& fr = frameResources_[dxCommon_->GetFrameIndex()];
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::Camera, fr.frameData.camera);
    commandList_->SetGraphicsRootConstantBufferView((UINT)RootSlot::LightCommon, fr.frameData.lightCommon);

    // 点光源、スポットライト、面光源を１つのテーブル（Slot 6）で一括設定
    commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::Lights, fr.lightSrvHandle);

    // シャドウマップをバインド (Slot 10 / register t5) - シャドウパス中はバインドしない
    ShadowMap* shadowMap = GetShadowMap();
    if (shadowMap && !isShadowPass_) {
        commandList_->SetGraphicsRootDescriptorTable((UINT)RootSlot::ShadowMap, shadowMap->GetSrvHandle());
    }
}

void DrawManager::BeginShadowPass() {
    ShadowMap* shadowMap = GetShadowMap();
    if (!shadowMap) return;
    isShadowPass_ = true;

    // 1. シャドウマップの準備 (バリア遷移、クリア、DSVセット)
    shadowMap->Begin(commandList_);

    // 2. ライト行列を定数バッファに反映
    auto& fr = frameResources_[dxCommon_->GetFrameIndex()];
    if (fr.lightCommonData) {
        // すでに SetFrameData で計算済みだが、念のため最新の状態を反映
        fr.lightCommonData->viewProjection = shadowMap->GetViewProjection();
    }

    // 3. DescriptorHeap再設定
    ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
    commandList_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // 4. バインド (ライト行列を定数バッファに反映させるため)
    BindCommonParameters();
}

void DrawManager::EndShadowPass() {
    ShadowMap* shadowMap = GetShadowMap();
    if (!shadowMap) return;
    isShadowPass_ = false;

    // 1. バリア遷移を元に戻す (DepthWrite -> SRV)
    shadowMap->End(commandList_);

    // 2. レンダーターゲットを復帰させる
    if (currentRenderTexture_) {
        // 元の RenderTexture があればそれを再設定 (クリアはしない)
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = currentRenderTexture_->GetRtvHandle();
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDSVCPUDescriptorHandle(0);
        commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

        // ビューポート等も復帰
        D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(currentRenderTexture_->GetWidth()), static_cast<float>(currentRenderTexture_->GetHeight()), 0.0f, 1.0f };
        D3D12_RECT scissor{ 0, 0, static_cast<long>(currentRenderTexture_->GetWidth()), static_cast<long>(currentRenderTexture_->GetHeight()) };
        commandList_->RSSetViewports(1, &viewport);
        commandList_->RSSetScissorRects(1, &scissor);
    }
    else {
        // なければバックバッファに戻す
        SetRenderTargetToBackBuffer(true);
    }
}

void DrawManager::ExecuteRenderQueues(IrufemiEngine* engine) {
    // 1. Skybox
    if (!skyboxQueue_.empty()) {
        engine->ApplySkyboxPSO();
        for (const auto& p : skyboxQueue_) {
            DrawSkybox(p);
        }
    }
    
    // Helper lambda to apply PSO efficiently
    auto DrawWithPSO = [&](auto& queue, auto drawFunc, bool isParticle = false, bool isSprite = false, bool isLine = false) {
        if (queue.empty()) return;
        
        BlendMode currentBlend = BlendMode::kBlendModeNormal;
        PSOManager::DepthWrite currentDepth = PSOManager::DepthWrite::Enable;
        PSOManager::CullMode currentCull = PSOManager::CullMode::Back;
        bool first = true;
        
        for (const auto& p : queue) {
            if (first || p.blendMode != currentBlend || p.depthWrite != currentDepth || p.cullMode != currentCull) {
                engine->SetBlend(p.blendMode);
                engine->SetDepthWrite(p.depthWrite);
                engine->SetCull(p.cullMode);
                if (isParticle) engine->ApplyParticlePSO();
                else if (isSprite) engine->ApplySpritePSO();
                else if (isLine) engine->ApplyLineInstancedPSO();
                else engine->ApplyPSO(); // Standard3D or Region
                
                currentBlend = p.blendMode;
                currentDepth = p.depthWrite;
                currentCull = p.cullMode;
                first = false;
            }
            drawFunc(p);
        }
    };
    
    // 2. Standard 3D (Opaque and Alpha blend)
    DrawWithPSO(standard3DQueue_, [&](const Standard3DPacket& p) { DrawStandard3D(p); });
    
    // 3. Region
    DrawWithPSO(regionQueue_, [&](const RegionPacket& p) { DrawRegion(p); });

    // 3.5 ModelRegion
    DrawWithPSO(modelRegionQueue_, [&](const ModelRegionPacket& p) { DrawModelRegion(p); });
    
    // 4. Line
    DrawWithPSO(lineQueue_, [&](const LinePacket& p) { DrawLineInstanced(p); }, false, false, true);

    // 5. Particles
    DrawWithPSO(particleQueue_, [&](const ParticlePacket& p) { DrawParticle(p); }, true);

    // 6. GPU Particles
    if (!gpuParticleQueue_.empty()) {
        BlendMode currentBlend = BlendMode::kBlendModeNormal;
        PSOManager::DepthWrite currentDepth = PSOManager::DepthWrite::Enable;
        PSOManager::CullMode currentCull = PSOManager::CullMode::Back;
        bool first = true;
        for (const auto& p : gpuParticleQueue_) {
            if (first || p.blendMode != currentBlend || p.depthWrite != currentDepth || p.cullMode != currentCull) {
                engine->SetBlend(p.blendMode);
                engine->SetDepthWrite(p.depthWrite);
                engine->SetCull(p.cullMode);
                engine->ApplyGpuParticlePSO();
                currentBlend = p.blendMode; currentDepth = p.depthWrite; currentCull = p.cullMode;
                first = false;
            }
            DrawGPUParticle(p);
        }
    }
    // 7. Voxel Particles
    if (!voxelParticleQueue_.empty()) {
        for (const auto& p : voxelParticleQueue_) {
            DrawVoxelParticle(p);
        }
    }

    // 8. Sprites
    DrawWithPSO(spriteQueue_, [&](const SpritePacket& p) { DrawSprite(p); }, false, true);

    // 8.5 UI 3D Objects (Always drawn on top of Sprites)
    DrawWithPSO(ui3DQueue_, [&](const Standard3DPacket& p) { DrawStandard3D(p); });

    // 9. Post Custom Draws
    for (auto& func : postRenderQueue_) {
        func();
    }

    ClearRenderQueues();
}

void DrawManager::ClearRenderQueues() {
    standard3DQueue_.clear();
    ui3DQueue_.clear();
    spriteQueue_.clear();
    particleQueue_.clear();
    lineQueue_.clear();
    gpuParticleQueue_.clear();
    voxelParticleQueue_.clear();
    skyboxQueue_.clear();
    regionQueue_.clear();
    modelRegionQueue_.clear();
    postRenderQueue_.clear();
}
