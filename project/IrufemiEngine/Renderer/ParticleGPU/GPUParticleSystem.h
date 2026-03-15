#pragma once

#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Type/PerFrame.h"
#include "Engine/Core/Type/PerView.h"
#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <random>

// 前方宣言
class DirectXCommon;
class DrawManager;
class TextureManager;
class Camera;
class IrufemiEngine;

struct ParticleCS {
    Vector3 translate;
    Vector3 scale;
    float lifeTime;
    Vector3 velocity;
    float currentTime;
    Vector4 color;
};

struct ParticleGPUMaterial {
    Vector4 color;
    int32_t useClampSampler = 0; // 0: WRAP, 1: CLAMP
    float pad[3];
    Matrix4x4 uvTransform;
};

struct EmitterSphere {
    // 位置
    Vector3 translate;
    // 射出半径
    float radius;
    // 射出数
    uint32_t count;
    // 射出間隔
    float frequency;
    // 射出間隔調整用時間
    float frequencyTime;
    // 射出許可
    uint32_t emit;
};

class GPUParticleSystem
{
public:
    // コンストラクタ
    GPUParticleSystem();
    // デストラクタ
    ~GPUParticleSystem();
    // 初期化
    void Initialize(Camera* camera, const std::string& textureName = "resources/circle.png");
    // 更新
    void Update();
    // 描画
    void Draw();
    // デバッグ
    void Debug();

    static void SetDXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
    static void SetDrawManager(DrawManager* drawManager) { drawManager_ = drawManager; }
    static void SetTextureManager(TextureManager* textureManager) { textureManager_ = textureManager; }
    static void SetEngine(IrufemiEngine* engine) { engine_ = engine; }

private:
    static DirectXCommon* dxCommon_;
    static DrawManager* drawManager_;
    static TextureManager* textureManager_;
    static IrufemiEngine* engine_;

    static const uint32_t kMaxParticles = 1024;

    EmitterSphere* emitterSphere_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> emitterSphereResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE emitterSphereUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE emitterSphereUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE emitterSphereSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE emitterSphereSrvHandleGPU_{};

    PerFrame* perFrameData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> perFrameResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE perFrameUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE perFrameUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE perFrameSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE perFrameSrvHandleGPU_{};

    ParticleCS* particleData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> particleResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE particleUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE particleUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE particleSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE particleSrvHandleGPU_{};

    int32_t* freeListIndex_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListIndexResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE freeListIndexUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListIndexUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE freeListIndexSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListIndexSrvHandleGPU_{};

    int32_t* freeList_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE freeListUavHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListUavHandleGPU_{};
    D3D12_CPU_DESCRIPTOR_HANDLE freeListSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE freeListSrvHandleGPU_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource_;
    PerView* perViewData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    ParticleGPUMaterial* materialData_ = nullptr;

    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};

    Camera* camera_ = nullptr;
};

