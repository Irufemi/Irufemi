#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <array>
#include <memory>
#include "Engine/Graphics/Data/VertexData.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include <mutex>

// 前方宣言
class DirectXCommon;
class DrawManager;
class DescriptorPool;

/**
 * @class DebugPrimitiveRenderer
 * @brief GPUインスタンシングを用いた高速なデバッグ用プリミティブ描画クラス
 * @details 以前はシングルトンでしたが、破棄順序の安全性を確保するため IrufemiEngine 管理に変更されました。
 */
class DebugPrimitiveRenderer {
public:
    /**
     * @brief コンストラクタ
     */
    DebugPrimitiveRenderer() = default;

    ~DebugPrimitiveRenderer();

    /**
     * @brief Initialize を実行する。
     */
    void Initialize(DirectXCommon* dx, DrawManager* drawM, DescriptorPool* srvAlloc);
    
    /**
     * @brief Update を実行する。
     */
    void Update();

    /**
     * @brief AddSphere を実行する。
     */
    void AddSphere(const Irufemi::Vector3& center, float radius, const Irufemi::Vector4& color);
    /**
     * @brief AddCube を実行する。
     */
    void AddCube(const Irufemi::Matrix4x4& transform, const Irufemi::Vector4& color);
    
    /**
     * @brief ClearInstances を実行する。
     */
    void ClearInstances();
    /**
     * @brief BuildInstanceBuffer を実行する。
     */
    void BuildInstanceBuffer();
    /**
     * @brief Draw を実行する。
     */
    void Draw();

private:
    struct InstanceData {
        Irufemi::Matrix4x4 world;
        Irufemi::Vector4 color;
    };

    /**
     * @brief CreateSphereResource を実行する。
     */
    void CreateSphereResource();
    /**
     * @brief CreateCubeResource を実行する。
     */
    void CreateCubeResource();
    /**
     * @brief EnsureInstancingSRVs を実行する。
     */
    void EnsureInstancingSRVs();

    DirectXCommon* dx_ = nullptr;
    DrawManager* drawManager_ = nullptr;
    DescriptorPool* srvAllocator_ = nullptr;

    // --- Irufemi::Sphere Data ---
    Microsoft::WRL::ComPtr<ID3D12Resource> sphereVertexResource_;
    D3D12_VERTEX_BUFFER_VIEW sphereVBV_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> sphereIndexResource_;
    D3D12_INDEX_BUFFER_VIEW sphereIBV_{};
    uint32_t sphereIndexCount_ = 0;

    std::vector<InstanceData> sphereInstances_;
    size_t activeSphereCount_ = 0;
    size_t maxSphereInstances_ = 65535;

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> sphereInstanceBuffer_;
    std::array<InstanceData*, kMaxFramesInFlight> sphereInstanceDataMap_{};
    std::array<uint32_t, kMaxFramesInFlight> sphereInstanceCapacity_{};
    std::array<uint32_t, kMaxFramesInFlight> sphereSrvIndex_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight> sphereSrvGPU_{};

    // --- Cube Data ---
    Microsoft::WRL::ComPtr<ID3D12Resource> cubeVertexResource_;
    D3D12_VERTEX_BUFFER_VIEW cubeVBV_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> cubeIndexResource_;
    D3D12_INDEX_BUFFER_VIEW cubeIBV_{};
    uint32_t cubeIndexCount_ = 0;

    std::vector<InstanceData> cubeInstances_;
    size_t activeCubeCount_ = 0;
    size_t maxCubeInstances_ = 65535;

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> cubeInstanceBuffer_;
    std::array<InstanceData*, kMaxFramesInFlight> cubeInstanceDataMap_{};
    std::array<uint32_t, kMaxFramesInFlight> cubeInstanceCapacity_{};
    std::array<uint32_t, kMaxFramesInFlight> cubeSrvIndex_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight> cubeSrvGPU_{};

    uint32_t lastUpdateFrameIndex_ = 0;
    
    std::mutex mutex_;
};
