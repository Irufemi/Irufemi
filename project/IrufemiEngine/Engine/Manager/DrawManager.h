#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <array>
#include <wrl.h>
#include "Renderer/TransformationMatrix.h"
#include "Engine/Graphics/Data/PointLight.h"
#include "Engine/Graphics/Data/SpotLight.h"
#include "Engine/Graphics/Data/AreaLight.h"
#include "Engine/Graphics/DirectX/RenderTexture.h"
#include "Engine/Core/Math/Vector4.h"
#include <vector>
#include <memory>

// 前方宣言
class DirectXCommon;
class Sprite;
class TriangleClass;
class SphereClass;
class ObjClass;
class ParticleSystem;
class CylinderClass;
class D3D12ResourceUtil;
class D3D12ResourceUtilLine;
class ModelRegion;
class SphereRegion;
class TetraRegion;
class SpriteRegion;
struct GpuMesh;
struct ManagedModel;
class Line2DClass;
class Line3DClass;
class Line3DRegion;
class CubeClass;
class Skybox;
struct SkinCluster;
struct GpuMaterial;

// 構造体を前方宣言
struct DirectionalLight;
struct CameraForGPU;

//描画のCommandListを積む順番
// Viewport → RootSignature → Pipeline → Topology → Buffers → CBV → SRV → Draw

class DrawManager {
private:

    DirectXCommon* dxCommon_ = nullptr;
    ID3D12GraphicsCommandList* commandList_ = nullptr; // コマンドリストをキャッシュ

    // シェーダーで定義したライトの最大数
    static const int kMaxPointLights = 4;
    static const int kMaxSpotLights = 4;
    static const int kMaxAreaLights = 4;

    // ライト配列を格納する構造体
    struct PointLights {
        PointLight lights[kMaxPointLights];
    };
    struct SpotLights {
        SpotLight lights[kMaxSpotLights];
    };
    struct AreaLights {
        AreaLight lights[kMaxAreaLights];
    };

    // カメラやライトの定数バッファを一時的に保持するリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> frameResource_;
    struct FrameData {
        D3D12_GPU_VIRTUAL_ADDRESS camera;
        D3D12_GPU_VIRTUAL_ADDRESS directionalLight;
        D3D12_GPU_VIRTUAL_ADDRESS pointLights;
        D3D12_GPU_VIRTUAL_ADDRESS spotLights;
        D3D12_GPU_VIRTUAL_ADDRESS areaLights;
    } frameData_{};
    CameraForGPU* cameraData_ = nullptr;
    DirectionalLight* directionalLightData_ = nullptr;
    PointLights* pointLightsData_ = nullptr;
    SpotLights* spotLightsData_ = nullptr;
    AreaLights* areaLightsData_ = nullptr;

    D3D12_GPU_DESCRIPTOR_HANDLE environmentMapHandle_{}; // 環境マップ用SRVハンドル

public: //メンバ関数

    void Initialize(DirectXCommon* dx);
    void Finalize();

    void BindPSO(ID3D12PipelineState* pso);

    void PreDraw(
        std::array<float, 4> clearColor = { 0.1f, 0.25f, 0.5f, 1.0f },
        float clearDepth = 1.0f,
        uint8_t clearStencil = 0
    );
    void PostDraw();

    // RenderTexture への描画開始
    void BeginRenderTexture(class RenderTexture* rt, const struct Vector4& clearColor);
    // RenderTexture への描画終了
    void EndRenderTexture(class RenderTexture* rt);

    // レンダーターゲットをバックバッファに戻す
    void SetRenderTargetToBackBuffer();

    // フレーム単位の共通データを設定
    void SetFrameData(const CameraForGPU& camera, const DirectionalLight& light, const std::vector<PointLight*>& pointLights, const std::vector<SpotLight*>& spotLights, const std::vector<AreaLight*>& areaLights);

    // 環境マップ設定用
    void SetEnvironmentMap(D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle);
    D3D12_GPU_DESCRIPTOR_HANDLE GetEnvironmentMap() const { return environmentMapHandle_; }

    void DrawTriangle(
        TriangleClass* triangle
    );

    void DrawParticle(ParticleSystem* resource);

    void DrawModelRegion(ModelRegion* region);

    void DrawRegion(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, const D3D12_GPU_DESCRIPTOR_HANDLE& textureHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& instancingSrvHandleGPU, const UINT& indexCount, const UINT& instanceCount);

    // LineInstancedシェーダー用描画関数
    void DrawLineInstanced(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, const D3D12_GPU_DESCRIPTOR_HANDLE& instancingSrvHandleGPU, const UINT& instanceCount);

    // Object3Dシェーダー用描画関数
    void DrawObject3D(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, const UINT& indexCount);

    // Object2Dシェーダー用描画関数
    void DrawObject2D(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, const UINT& indexCount);

    // Skyboxシェーダー用描画関数
    void DrawSkybox(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, const UINT& indexCount);

    // モデル描画用の関数
    void DrawModel(const ManagedModel* model, D3D12_GPU_VIRTUAL_ADDRESS transformGpuVA, const std::vector<std::shared_ptr<GpuMaterial>>& materials);

    void DrawAnimationModel(const ManagedModel* model, D3D12_GPU_VIRTUAL_ADDRESS transformGpuVA, const SkinCluster& skinCluster, const D3D12_GPU_DESCRIPTOR_HANDLE& skinnedVertexSrv, D3D12_GPU_VIRTUAL_ADDRESS skinningInfoGpuVA, uint32_t numVertices, const std::vector<std::shared_ptr<GpuMaterial>>& materials);

    void DispatchSkinning(const D3D12_GPU_DESCRIPTOR_HANDLE& palette, const D3D12_GPU_DESCRIPTOR_HANDLE& inputVertex, const D3D12_GPU_DESCRIPTOR_HANDLE& influence, const D3D12_GPU_DESCRIPTOR_HANDLE& outputVertex, const D3D12_GPU_VIRTUAL_ADDRESS& skinningInformation, const float& verticesSize);

    void DrawParticleGPU(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_GPU_VIRTUAL_ADDRESS& material, const D3D12_GPU_VIRTUAL_ADDRESS& perView, const D3D12_GPU_DESCRIPTOR_HANDLE& textureHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& particleSrv, const UINT& instanceCount);

    DirectXCommon* GetDxCommon() const { return dxCommon_; }
};