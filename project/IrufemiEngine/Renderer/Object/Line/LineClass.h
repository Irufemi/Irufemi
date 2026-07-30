#include "../../System/Core/IRenderable.h"
#pragma once

#include <d3d12.h>
#include <memory>
#include <cstdint>
#include <random>
#include <array>
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Transform.h"
#include "../../../Engine/Graphics/Data/TransformationMatrix.h"
#include "Renderer/System/Core/LineResource.h"

// 前方宣言
class Camera;
class DrawManager;
class DirectXCommon;
class DescriptorPool;

// 3Dラインのインスタンス描画用クラス
class Line3DBatch : public IRenderable {
public:
    Line3DBatch() {
        instancingSrvIndex_.fill(UINT32_MAX);
    }
    ~Line3DBatch();

    /**
     * @brief Initialize を実行する。
     */
    void Initialize();
    /**
     * @brief Update を実行する。
     */
    void Update();
    /**
     * @brief AddInstance を実行する。
     */
    void AddInstance(const Irufemi::Vector3& start, const Irufemi::Vector3& end, const Irufemi::Vector4& color, float life = 1.0f);
    /**
     * @brief ClearInstances を実行する。
     */
    void ClearInstances();
    /**
     * @brief BuildInstanceBuffer を実行する。
     */
    void BuildInstanceBuffer(bool force = false);
    /**
     * @brief SyncBeforeDraw を実行する。
     */
    void SyncBeforeDraw() override;
    /**
     * @brief Draw を実行する。
     */
    void Draw() override;

    // --- DrawManager から参照する Getter 群 ---
    /**
     * @brief BaseResource を取得する。
     * @return 取得された BaseResource
     */
    LineResource* GetBaseResource() const { return baseLineResource_.get(); }
    /**
     * @brief InstancingSrvHandleGPU を取得する。
     * @return 取得された InstancingSrvHandleGPU
     */
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvGPU_[lastUpdateFrameIndex_]; }
    /**
     * @brief InstanceCountU32 を取得する。
     * @return 取得された InstanceCountU32
     */
    UINT GetInstanceCountU32() const { return static_cast<UINT>(activeCount_); }

    /**
     * @brief DepthWrite を設定する。
     * @param[in] depthWrite 設定する DepthWrite の値
     */
    void SetDepthWrite(PSOManager::DepthWrite depthWrite) { depthWrite_ = depthWrite; }
    /**
     * @brief DepthWrite を取得する。
     * @return 取得された DepthWrite
     */
    PSOManager::DepthWrite GetDepthWrite() const { return depthWrite_; }

    // 依存注入
    /**
     * @brief DirectXCommon を設定する。
     * @param[in] dx 設定する DirectXCommon の値
     */
    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    /**
     * @brief SrvAllocator を設定する。
     * @param[in] alloc 設定する SrvAllocator の値
     */
    static void SetSrvAllocator(DescriptorPool* alloc) { s_srvAllocator_ = alloc; }
    /**
     * @brief DrawManager を設定する。
     * @param[in] drawM 設定する DrawManager の値
     */
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    /**
     * @brief Engine を設定する。
     * @param[in] engine 設定する Engine の値
     */
    static void SetEngine(class IrufemiEngine* engine) { engine_ = engine; }

private:
    struct LineInstance {
        Irufemi::Vector3 start;
        Irufemi::Vector3 end;
        Irufemi::Vector4 color;
        float life = 1.0f;
        float age = 0.0f;
        bool active = false;
    };

    // VS 側が読む1インスタンス分のデータ
    struct InstanceData {
        Irufemi::Vector4 start;
        Irufemi::Vector4 end;
        Irufemi::Vector4 color;
    };

    /**
     * @brief CreateOrResizeInstanceBuffer を実行する。
     */
    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);
    /**
     * @brief EnsureInstancingSRV を実行する。
     */
    void EnsureInstancingSRV();

private:
    // 静的依存
    static DirectXCommon* dx_;
    static DrawManager* drawManager_;
    static DescriptorPool* s_srvAllocator_;
    static class IrufemiEngine* engine_;

    std::unique_ptr<LineResource> baseLineResource_ = nullptr;

    std::vector<LineInstance> instances_;
    size_t activeCount_ = 0;
    size_t maxInstances_ = 65535;

    // インスタンシング用 StructuredBuffer と SRV
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> instanceBuffer_;
    std::array<InstanceData*, kMaxFramesInFlight> instanceData_{};
    std::array<uint32_t, kMaxFramesInFlight> instanceCapacity_{};

    uint32_t lastUpdateFrameIndex_ = 0;
    bool isDirty_ = true;

    std::array<uint32_t, kMaxFramesInFlight> instancingSrvIndex_{};
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight> instancingSrvCPU_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight> instancingSrvGPU_{};

    PSOManager::DepthWrite depthWrite_ = PSOManager::DepthWrite::Enable;
};
