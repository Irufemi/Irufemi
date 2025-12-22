#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <array>
#include <wrl.h>
#include "math/TransformationMatrix.h" // 追加

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
struct PointLight;
class PointLightClass;
struct SpotLight;
class SpotLightClass;
class SpriteRegion;
struct GpuMesh;
struct ManagedModel;
class Line2DClass;
class Line3DClass;
class CubeClass;

//描画のCommandListを積む順番
// Viewport → RootSignature → Pipeline → Topology → Buffers → CBV → SRV → Draw

class DrawManager {
private:

    DirectXCommon* dxCommon_ = nullptr;

    // カメラやライトの定数バッファを一時的に保持するリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> frameResource_;
    struct FrameData {
        D3D12_GPU_VIRTUAL_ADDRESS camera;
        D3D12_GPU_VIRTUAL_ADDRESS directionalLight;
    } frameData_{};
    CameraForGPU* cameraData_ = nullptr;
    DirectionalLight* directionalLightData_ = nullptr;


    PointLightClass* pointLight_ = nullptr;

    SpotLightClass* spotLight_ = nullptr;

    void EnsurePointLightResource(); // 生成・初期化の遅延実行用
    void EnsureSpotLightResource(); // 生成・初期化の遅延実行用

    // フォールバック込みで今フレーム使うGPUアドレスを取得
    D3D12_GPU_VIRTUAL_ADDRESS GetPointLightVA();
    D3D12_GPU_VIRTUAL_ADDRESS GetSpotLightVA();


public: //メンバ関数

    void Initialize(DirectXCommon* dx);
    void Finalize();

    // 追加（保持はしないで即時バインド）
    void BindPSO(ID3D12PipelineState* pso);

    void PreDraw(
        std::array<float, 4> clearColor = { 0.1f, 0.25f, 0.5f, 1.0f },
        float clearDepth = 1.0f,
        uint8_t clearStencil = 0
    );
    void PostDraw();

    // フレーム単位の共通データを設定
    void SetFrameData(const CameraForGPU& camera, const DirectionalLight& light);

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

    // モデル描画用の新関数
    void DrawModel(const ManagedModel* model, const TransformationMatrix& matrix);

    void SetPointLightClass(PointLightClass* pointLightClass) { pointLight_ = pointLightClass; }
    void SetPointLight(PointLight& info);

    void SetSpotLightClass(SpotLightClass* spotLightClass) { spotLight_ = spotLightClass; }
    void SetSpotLight(SpotLight& info);

    void DrawSpriteRegion(SpriteRegion* region);
    void DrawSharedMesh(const GpuMesh* gpuMesh, D3D12ResourceUtil* instanceResource);
};