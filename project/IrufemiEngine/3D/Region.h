#pragma once

#include <vector>
#include <memory>
#include <string>
#include <wrl.h>
#include <d3d12.h>

#include "function/Function.h"      // VertexData / ObjModel / ObjMesh / ObjMaterial など
#include "function/Math.h"          // Math::MakeAffineMatrix ほか
#include "math/Transform.h"         // Transform

class Camera;
class DirectXCommon;
class TextureManager;
class DrawManager;
class DescriptorAllocator; // 追加

class Region {
public:
    // 初期化：OBJ 読み込みと共有テクスチャの取得
    void Initialize(
        Camera* camera,
        const std::string& objFilename);

    // インスタンスを追加（Transformを保持）
    void AddInstance(const Transform& t);

    // 全インスタンス削除
    void ClearInstances();

    // インスタンシングバッファ更新（必要時だけ再構築/更新）
    void BuildInstanceBuffer(bool force = false);

    // 描画（事前に DrawManager::PreDraw 済みであること）
    void Draw();

    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }

    // 追加: SRV アロケータ注入
    static void SetSrvAllocator(DescriptorAllocator* alloc) { srvAllocator_ = alloc; }

    // --- 追加: DrawManager から参照する Getter 群 ---
    D3D12_VERTEX_BUFFER_VIEW&       GetVertexBufferView() { return vertexBufferView_; }
    ID3D12Resource*                 GetMaterialResource() { return materialResource_.Get(); }
    ID3D12Resource*                 GetDirectionalLightResource() { return directionalLightResource_.Get(); }
    ID3D12Resource*                 GetCameraResource() { return cameraResource_.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE     GetTextureHandle() const { return textureHandle_; }
    D3D12_GPU_DESCRIPTOR_HANDLE     GetInstancingSrvHandleGPU() const { return instancingSrvGPU_; }
    UINT                            GetVertexCount() const { return vertexCount_; }
    UINT                            GetInstanceCount() const { return static_cast<UINT>(instances_.size()); }

private:
    struct InstanceData {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
        Vector4   color; // 使わない場合は {1,1,1,1}
    };

    // リソース生成ヘルパ
    void CreateMeshBuffers(const ObjMesh& mesh);
    void CreateMaterialResources(const ObjMesh& mesh);
    void EnsureSharedTexture(const ObjMesh& mesh);
    void EnsureLightAndCamera();
    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);

private:
    static DirectXCommon*  dx_;
    static TextureManager* textureManager_;
    static DrawManager*    drawManager_;
    static DescriptorAllocator* srvAllocator_; // 追加

    Camera* camera_ = nullptr;

    ObjModel objModel_{};

    // メッシュ（単一メッシュ想定）
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW               vertexBufferView_{};
    UINT                                   vertexCount_ = 0;

    // マテリアル/ライト/カメラ
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;

    // テクスチャ（共有SRV）
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};

    // インスタンシング用 StructuredBuffer と SRV
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
    D3D12_CPU_DESCRIPTOR_HANDLE            instancingSrvCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE            instancingSrvGPU_{};
    uint32_t                               instancingSrvIndex_ = UINT32_MAX; // 1ディスクリプタ固定で再利用

    // インスタンス（Transform を保持）
    std::vector<Transform> instances_;
    bool                   instanceDirty_ = false;
};