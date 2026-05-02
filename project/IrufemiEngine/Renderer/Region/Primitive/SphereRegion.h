#include "../../Core/IRenderable.h"
#pragma once

#include <vector>
#include <memory>
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include <cassert>
#include <array>
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/DirectX/DynamicConstantBuffer.h"

#include "../../VertexData.h"            // VertexData
#include "../../../Engine/Graphics/Data/Material.h"              // Material
#include "../../../Engine/Core/Math/Math.h"              // MakeAffineMatrix など
#include "../../../Engine/Core/Math/Transform.h"             // Transform
#include "../../../Engine/Graphics/Data/DirectionalLight.h"      // DirectionalLight
#include "../../../Engine/Graphics/Data/CameraForGPU.h"          // CameraForGPU

class DirectXCommon;
class Camera;
class TextureManager;
class DrawManager;
class DescriptorPool; // 追加

class SphereRegion : public IRenderable {
public:
    SphereRegion() {
        instancingSrvIndex_.fill(UINT32_MAX);
    }

    // 静的セットアップ
    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetSrvAllocator(DescriptorPool* alloc) { srvPool_ = alloc; } // 追加
    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    bool IsCullingEnabled() const { return isCullingEnabled_; }
    void SetCastShadows(bool cast) { castShadows_ = cast; }
    bool GetCastShadows() const { return castShadows_; }

    ~SphereRegion(); // 追加(遅延解放で返却)

    // 初期化：スフィアメッシュ生成 + マテリアル/ライト/カメラ + 共有テクスチャ
    // subdivision: 緯度経度の分割数(SphereClass と同等の 16 を既定)
    void Initialize(Camera* camera, const std::string& textureName  = "resources/uvChecker.png", uint32_t subdivision = 16);

    // インスタンス追加(Transform 直接)
    void AddInstance(const Transform& t);
    // 色つきインスタンス追加(Transform 直接)
    void AddInstance(const Transform& t, const Vector4& color);
    // インスタンス追加(中心と半径の簡易API)
    void AddInstance(const Vector3& center, float radius, const Vector3& rotate = { 0,0,0 });
    // 色つきインスタンス追加(中心と半径の簡易API)
    void AddInstance(const Vector3& center, float radius, const Vector3& rotate, const Vector4& color);

    // インスタンス更新
    void UpdateInstance(uint32_t index, const Transform& t);

    // 全インスタンス削除
    void ClearInstances();

    // インスタンスバッファ更新(force=true で毎フレーム更新)
    void BuildInstanceBuffer(bool force = false);

    // 描画(事前に DrawManager::PreDraw 済みであること)
    void SyncBeforeDraw() override;
    void Draw() override;

    // 色設定(マテリアル全体 or インスタンス個別/一括)
    void SetColor(const Vector4& color);                 // マテリアル色(全体に乗算される前提の色)
    void SetEnvironmentCoefficient(float coefficient); 
    void SetInstanceColor(uint32_t index, const Vector4& color); // 個別インスタンス色
    void SetAllInstanceColor(const Vector4& color);      // 全インスタンス同色

    // DrawManager 用 Getter
    D3D12_VERTEX_BUFFER_VIEW&   GetVertexBufferView() { return vertexBufferView_; }
    D3D12_INDEX_BUFFER_VIEW&    GetIndexBufferView() { return indexBufferView_; }
    D3D12_GPU_VIRTUAL_ADDRESS   GetMaterialVAddress() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle() const { return textureHandle_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvGPU_[lastUpdateFrameIndex_]; }
    UINT                        GetIndexCount() const { return indexCount_; }
    UINT                        GetInstanceCount() const { return visibleInstanceCount_; }

private:
    struct InstanceData {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
        Vector4   color; // インスタンス色
    };

    // メッシュ生成(単位球)
    void BuildSphereMesh(uint32_t subdivision, std::vector<VertexData>& outVertices, std::vector<uint32_t>& outIndices);

    // リソース群作成
    void CreateMeshBuffers(const std::vector<VertexData>& vertices, const std::vector<uint32_t>& indices);
    void CreateMaterialResources();
    void EnsureSharedTexture(const std::string& textureName);
    void EnsureLightAndCamera();
    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);

private:
    static DirectXCommon* dx_;
    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DescriptorPool* srvPool_; // 追加

    Camera* camera_ = nullptr;

    // メッシュ(VB/IB)
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW               vertexBufferView_{};
    UINT                                   vertexCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW                indexBufferView_{};
    UINT                                   indexCount_ = 0;

    // マテリアル
    uint32_t materialCbIndex_ = static_cast<uint32_t>(-1);
    Material cpuMaterialData_{};

    // 共有テクスチャ SRV
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};

    // インスタンシング StructuredBuffer と SRV
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> instanceBuffer_{};
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            instancingSrvCPU_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            instancingSrvGPU_{};
    std::array<uint32_t, kMaxFramesInFlight>                               instancingSrvIndex_{};

    // インスタンス群(Transform / Color を保持)
    std::vector<Transform> instances_;
    std::vector<Vector4>   instanceColors_;
    bool                   instanceDirty_ = false;
    bool                   isCullingEnabled_ = true;
    uint32_t               visibleInstanceCount_ = 0;
    
    uint32_t lastUpdateFrameIndex_ = 0;
    bool isDirty_ = true;
    bool castShadows_ = true;
};

