#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <array>
#include <wrl.h>
#include "math/TransformationMatrix.h" // 追加
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/AreaLight.h"
#include <vector>

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
class Region;
class SphereRegion; 
class TetraRegion; 
class SpriteRegion;
struct GpuMesh;
struct ManagedModel;
class Line2DClass;
class Line3DClass;
class Line3DRegion;
class CubeClass;
struct SkinCluster;

// 構造体を前方宣言
struct DirectionalLight;
struct CameraForGPU;

//描画のCommandListを積む順番
// Viewport → RootSignature → Pipeline → Topology → Buffers → CBV → SRV → Draw

class DrawManager {
private:

    DirectXCommon* dxCommon_ = nullptr;

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


public: //メンバ関数

    void Initialize(DirectXCommon* dx);
    void Finalize();

    // 追加(保持はしないで即時バインド)
    void BindPSO(ID3D12PipelineState* pso);

    void PreDraw(
        std::array<float, 4> clearColor = { 0.1f, 0.25f, 0.5f, 1.0f },
        float clearDepth = 1.0f,
        uint8_t clearStencil = 0
    );
    void PostDraw();

    // フレーム単位の共通データを設定
    void SetFrameData(const CameraForGPU& camera, const DirectionalLight& light, const std::vector<PointLight*>& pointLights, const std::vector<SpotLight*>& spotLights, const std::vector<AreaLight*>& areaLights);

    void DrawTriangle(
        TriangleClass* triangle
    );

    void DrawCube(CubeClass* cube);

    void DrawSprite(Sprite* sprite);

    void DrawSphere(SphereClass* sphere);

    void DrawCylinder(CylinderClass* cylinder);

    void DrawParticle(ParticleSystem* resource);

    void DrawRegion(Region* region);

    void DrawSphereRegion(SphereRegion* region);

    void DrawTetraRegion(TetraRegion* region);

    void DrawByIndex(D3D12ResourceUtil* resource);

    void DrawByVertex(D3D12ResourceUtil* resource);

    void DrawLine2D(Line2DClass* line);

    void DrawLine3D(Line3DClass* line);

    void DrawLine3DRegion(Line3DRegion* region);

    // モデル描画用の新関数
    void DrawModel(const ManagedModel* model, D3D12_GPU_VIRTUAL_ADDRESS transformGpuVA);

    void DrawAnimationModel(const ManagedModel* model, D3D12_GPU_VIRTUAL_ADDRESS transformGpuVA, const SkinCluster& skinCluster);

    void DrawSpriteRegion(SpriteRegion* region);
    void DrawSharedMesh(const GpuMesh* gpuMesh, D3D12ResourceUtil* instanceResource);

    DirectXCommon* GetDxCommon() const { return dxCommon_; }
};