#pragma once
#include <string>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector3Int.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Resource/Model/Data/VoxelizedModel.h"

// 前方宣言
class IrufemiEngine;
class Camera;
class ModelManager;
class TextureManager;

// HLSL側のVoxelEmitter構造体と一致させる
struct VoxelEmitter {
    Vector3 emitPosition = { 0.0f, 0.0f, 0.0f };
    float time = 0.0f;
    float lifeTime = 3.0f;
    float gravity = 9.8f;
    uint32_t emit = 0;
    float dispersion = 5.0f;   // 爆発の散開係数
    float convergence = 1.0f;  // 収束係数
    float pad; // 16バイトアライメント
};

// HLSL側のPerView構造体と一致させる
struct PerView {
    Matrix4x4 viewProjection;
    Matrix4x4 billboard;
};

// HLSL側のVoxelParticle構造体と一致させる
struct VoxelParticle {
    Vector3 position;
    Vector3 velocity;
    Vector4 color;
    float life;
    float size;
    uint32_t isActive;
    float pad[2]; // アライメント調整
};


class VoxelParticleSystem {
public:
    VoxelParticleSystem() = default;
    ~VoxelParticleSystem() = default;

    static void SetEngine(IrufemiEngine* engine) { engine_ = engine; }

    void Initialize(
        const std::string& modelName,
        const Vector3Int& resolution,
        Camera* camera
    );

    void Update(float deltaTime);
    void Draw();

    void Emit(const Vector3& position);

private:
    void CreateResources();
    void CreatePSO();

private:
    Camera* camera_ = nullptr;
    ModelManager* modelManager_ = nullptr;
    TextureManager* textureManager_ = nullptr;
    ID3D12Device* device_ = nullptr;

    std::unique_ptr<VoxelizedModel> voxelModel_;

    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> voxelBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> emitterConstantBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> perViewConstantBuffer_;

    // デスクリプタハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE voxelSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE voxelSrvHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE particleSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE particleSrvHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE particleUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE particleUavHandleGPU_{};

    // PSO
    Microsoft::WRL::ComPtr<ID3D12PipelineState> initializePSO_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> updatePSO_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> drawPSO_;

    VoxelEmitter emitterData_{};
    VoxelEmitter* mappedEmitterData_ = nullptr;
    PerView* mappedPerViewData_ = nullptr;

    uint32_t voxelCount_ = 0;
    bool isEmitting_ = false;

    static IrufemiEngine* engine_;
};

