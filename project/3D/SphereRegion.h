#pragma once

#include <vector>
#include <memory>
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include <cassert>

#include "function/Function.h"          // VertexData
#include "function/Math.h"              // MakeAffineMatrix など
#include "math/Transform.h"             // Transform
#include "math/Material.h"              // Material
#include "math/DirectionalLight.h"      // DirectionalLight
#include "math/CameraForGPU.h"          // CameraForGPU

class DirectXCommon;
class Camera;
class TextureManager;
class DrawManager;
class DescriptorAllocator; // 追加

class SphereRegion {
public:
    // 静的セットアップ
    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetSrvAllocator(DescriptorAllocator* alloc) { srvAllocator_ = alloc; } // 追加

    ~SphereRegion(); // 追加（遅延解放で返却）

    // 初期化：スフィアメッシュ生成 + マテリアル/ライト/カメラ + 共有テクスチャ
    // subdivision: 緯度経度の分割数（SphereClass と同等の 16 を既定）
    void Initialize(Camera* camera, const std::string& textureName  = "resources/uvChecker.png", uint32_t subdivision = 16);

    // インスタンス追加（Transform 直接）
    void AddInstance(const Transform& t);
    // 色つきインスタンス追加（Transform 直接）
    void AddInstance(const Transform& t, const Vector4& color);
    // インスタンス追加（中心と半径の簡易API）
    void AddInstance(const Vector3& center, float radius, const Vector3& rotate = { 0,0,0 });
    // 色つきインスタンス追加（中心と半径の簡易API）
    void AddInstance(const Vector3& center, float radius, const Vector3& rotate, const Vector4& color);

    // 全インスタンス削除
    void ClearInstances();

    // インスタンスバッファ更新（force=true で毎フレーム更新）
    void BuildInstanceBuffer(bool force = false);

    // 描画（事前に DrawManager::PreDraw 済みであること）
    void Draw();

    // 色設定（マテリアル全体 or インスタンス個別/一括）
    void SetColor(const Vector4& color);                 // マテリアル色（全体に乗算される前提の色）
    void SetInstanceColor(uint32_t index, const Vector4& color); // 個別インスタンス色
    void SetAllInstanceColor(const Vector4& color);      // 全インスタンス同色

    // DrawManager 用 Getter
    D3D12_VERTEX_BUFFER_VIEW&   GetVertexBufferView() { return vertexBufferView_; }
    D3D12_INDEX_BUFFER_VIEW&    GetIndexBufferView() { return indexBufferView_; }
    ID3D12Resource*             GetMaterialResource() { return materialResource_.Get(); }
    ID3D12Resource*             GetDirectionalLightResource() { return directionalLightResource_.Get(); }
    ID3D12Resource*             GetCameraResource() { return cameraResource_.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle() const { return textureHandle_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvGPU_; }
    UINT                        GetIndexCount() const { return indexCount_; }
    UINT                        GetInstanceCount() const { return static_cast<UINT>(instances_.size()); }

private:
    struct InstanceData {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
        Vector4   color; // インスタンス色
    };

    // メッシュ生成（単位球）
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
    static DescriptorAllocator* srvAllocator_; // 追加

    Camera* camera_ = nullptr;

    // メッシュ（VB/IB）
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW               vertexBufferView_{};
    UINT                                   vertexCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW                indexBufferView_{};
    UINT                                   indexCount_ = 0;

    // マテリアル/ライト/カメラ
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;

    // 共有テクスチャ SRV
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};

    // インスタンシング StructuredBuffer と SRV
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
    D3D12_CPU_DESCRIPTOR_HANDLE            instancingSrvCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE            instancingSrvGPU_{};
    uint32_t                               instancingSrvIndex_ = UINT32_MAX; // 1スロット確保して再利用

    // インスタンス群（Transform / Color を保持）
    std::vector<Transform> instances_;
    std::vector<Vector4>   instanceColors_;
    bool                   instanceDirty_ = false;
};