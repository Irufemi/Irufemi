#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <array>
#include <wrl.h>

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
class SpriteRegion; // 追加
struct GpuMesh; // 追加
class Line2DClass;
class Line3DClass;

//描画のCommandListを積む順番
// Viewport → RootSignature → Pipeline → Topology → Buffers → CBV → SRV → Draw

class DrawManager {
private:

    DirectXCommon* dxCommon_ = nullptr;

    PointLightClass* pointLight_ = nullptr;

    SpotLightClass* spotLight_ = nullptr;

    void EnsurePointLightResource(); // 生成・初期化の遅延実行用
    void EnsureSpotLightResource(); // 生成・初期化の遅延実行用

    // フォールバック込みで今フレーム使うGPUアドレスを取得
    D3D12_GPU_VIRTUAL_ADDRESS GetPointLightVA();
    D3D12_GPU_VIRTUAL_ADDRESS GetSpotLightVA();


public: //メンバ関数

    void Initialize(DirectXCommon* dx) { dxCommon_ = dx; }
    void Finalize();

    // 追加（保持はしないで即時バインド）
    void BindPSO(ID3D12PipelineState* pso);

    void PreDraw(
        std::array<float, 4> clearColor = { 0.1f, 0.25f, 0.5f, 1.0f },
        float clearDepth = 1.0f,
        uint8_t clearStencil = 0
    );
    void PostDraw();

    void DrawTriangle(
        TriangleClass* triangle
    );

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

    void SetPointLightClass(PointLightClass* pointLightClass) { pointLight_ = pointLightClass; }
    void SetPointLight(PointLight& info);

    void SetSpotLightClass(SpotLightClass* spotLightClass) { spotLight_ = spotLightClass; }
    void SetSpotLight(SpotLight& info);

    void DrawSpriteRegion(SpriteRegion* region);
    void DrawSharedMesh(const GpuMesh* gpuMesh, D3D12ResourceUtil* instanceResource);
};