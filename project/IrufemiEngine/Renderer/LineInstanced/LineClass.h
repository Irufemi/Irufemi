#pragma once

#include <d3d12.h>
#include <memory>
#include <cstdint>
#include <vector>
#include <random>
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Transform.h"
#include "Renderer/TransformationMatrix.h"
#include "Renderer/LineInstanced/LineResource.h"

// 前方宣言
class Camera;
class DrawManager;
class DirectXCommon;
class DescriptorPool;

// 3Dラインのインスタンス描画用クラス
class Line3DRegion {
public:
    Line3DRegion() = default;
    ~Line3DRegion();

    void Initialize(Camera* camera);
    void Update();
    void AddInstance(const Vector3& start, const Vector3& end, const Vector4& color, float life = 1.0f);
    void ClearInstances();
    void BuildInstanceBuffer(bool force = false);
    void Draw();

    // --- DrawManager から参照する Getter 群 ---
    LineResource* GetBaseResource() const { return baseLineResource_.get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvGPU_; }
    UINT GetInstanceCountU32() const { return static_cast<UINT>(activeCount_); }

    // 依存注入
    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    static void SetSrvAllocator(DescriptorPool* alloc) { s_srvAllocator_ = alloc; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }

private:
    struct LineInstance {
        Vector3 start;
        Vector3 end;
        Vector4 color;
        float life = 1.0f;
        float age = 0.0f;
        bool active = false;
    };

    // VS 側が読む1インスタンス分のデータ
    struct InstanceData {
        Matrix4x4 WVP;
        Vector4 color;
    };

    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);
    void EnsureInstancingSRV();

private:
    // 静的依存
    static DirectXCommon* dx_;
    static DrawManager* drawManager_;
    static DescriptorPool* s_srvAllocator_;

    Camera* camera_ = nullptr;
    std::unique_ptr<LineResource> baseLineResource_ = nullptr;

    std::vector<LineInstance> instances_;
    size_t activeCount_ = 0;
    size_t maxInstances_ = 8192;

    // インスタンシング用 StructuredBuffer と SRV
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
    InstanceData* instanceData_ = nullptr;
    uint32_t instanceCapacity_ = 0;

    uint32_t instancingSrvIndex_ = UINT32_MAX;
    D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvGPU_{};
};

