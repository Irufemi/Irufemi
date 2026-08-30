#pragma once

#include "Renderer/System/Core/IRenderable.h"
#include <d3d12.h>
#include <vector>
#include <string>
#include <memory>
#include <array>
#include <wrl.h>
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Matrix4x4.h"
#include "Core/Math/Transform.h"
#include "RHI/DirectX12/DirectXCommon.h"
#include "Renderer/Data/RenderPackets.h"

// 前方宣言
class TextureManager;
class DrawManager;
class CameraManager;
class Object2DResource;
class DescriptorPool;
class Camera;

/**
 * @class SpriteBatch
 * @brief 2Dスプライトのインスタンシング描画（バッチ描画）を行うクラス
 */
class SpriteBatch : public IRenderable {
public:
    SpriteBatch();
    ~SpriteBatch() override;

    /**
     * @brief Initialize を実行する。
     */
    void Initialize(const std::string& textureName = "resources/uvChecker.png");
    /**
     * @brief Update を実行する。
     */
    void Update();

    // インスタンスの追加
    /**
     * @brief AddInstance を実行する。
     */
    void AddInstance(const Irufemi::Transform& transform, const Irufemi::Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f});
    /**
     * @brief AddInstance を実行する。
     */
    void AddInstance(const Irufemi::Vector2& position, const Irufemi::Vector2& size, float rotation = 0.0f, const Irufemi::Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f}, const Irufemi::Vector2& anchor = {0.5f, 0.5f});
    
    /**
     * @brief ClearInstances を実行する。
     */
    void ClearInstances();

    /**
     * @brief SyncBeforeDraw を実行する。
     */
    void SyncBeforeDraw() override;
    /**
     * @brief Draw を実行する。
     */
    void Draw() override;
    /**
     * @brief Draw を実行する。
     */
    void Draw(bool isTopMost);

    // Getters
    /**
     * @brief D3D12Resource を取得する。
     * @return 取得された D3D12Resource
     */
    Object2DResource* GetD3D12Resource() const { return baseResource_.get(); }
    /**
     * @brief InstancingSrvHandleGPU を取得する。
     * @return 取得された InstancingSrvHandleGPU
     */
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const;
    /**
     * @brief InstanceCount を取得する。
     * @return 取得された InstanceCount
     */
    UINT GetInstanceCount() const { return static_cast<UINT>(visibleInstanceCount_); }
    
    /**
     * @brief TopMost を設定する。
     * @param[in] isTopMost 設定する TopMost の値
     */
    void SetTopMost(bool isTopMost) { isTopMost_ = isTopMost; }
    /**
     * @brief CustomPSO を設定する。
     * @param[in] pso 設定する CustomPSO の値
     */
    void SetCustomPSO(ID3D12PipelineState* pso) { customPSO_ = pso; }
    /**
     * @brief CustomCBV を設定する。
     * @param[in] cbv 設定する CustomCBV の値
     */
    void SetCustomCBV(D3D12_GPU_VIRTUAL_ADDRESS cbv) { customCBVAddress_ = cbv; }

    /**
     * @brief TextureManager を設定する。
     * @param[in] tm 設定する TextureManager の値
     */
    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    /**
     * @brief DrawManager を設定する。
     * @param[in] dm 設定する DrawManager の値
     */
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    /**
     * @brief CameraManager を設定する。
     * @param[in] cm 設定する CameraManager の値
     */
    static void SetCameraManager(CameraManager* cm) { cameraManager_ = cm; }
    /**
     * @brief DirectXCommon を設定する。
     * @param[in] dx 設定する DirectXCommon の値
     */
    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    /**
     * @brief SrvAllocator を設定する。
     * @param[in] pool 設定する SrvAllocator の値
     */
    static void SetSrvAllocator(DescriptorPool* pool) { srvPool_ = pool; }

private:
    struct SpriteInstance {
        Irufemi::Transform transform;
        Irufemi::Vector4 color;
        Irufemi::Vector2 anchor; 
        Irufemi::Vector2 size;   
    };

    struct InstanceData {
        Irufemi::Matrix4x4 WVP;
        Irufemi::Vector4 color;
    };

    /**
     * @brief CreateOrResizeInstanceBuffer を実行する。
     */
    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);
    /**
     * @brief BuildInstanceBuffer を実行する。
     */
    void BuildInstanceBuffer(bool force = false);
    /**
     * @brief ApplyAnchorToVertices を実行する。
     */
    void ApplyAnchorToVertices();

private:
    std::unique_ptr<Object2DResource> baseResource_ = nullptr;
    std::vector<SpriteInstance> instances_;

    uint32_t visibleInstanceCount_ = 0;
    bool instanceDirty_ = false;
    bool isTopMost_ = false;
    Irufemi::Vector2 textureSize_{0.0f, 0.0f};
    
    ID3D12PipelineState* customPSO_ = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS customCBVAddress_ = 0;
    
    // インスタンシング用バッファ
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> instanceBuffer_;
    std::array<InstanceData*, kMaxFramesInFlight> instanceData_{};
    std::array<uint32_t, kMaxFramesInFlight> instanceCapacity_{};
    std::array<uint32_t, kMaxFramesInFlight> instancingSrvIndex_{};
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight> instancingSrvCPU_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight> instancingSrvGPU_{};

    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static CameraManager* cameraManager_;
    static DirectXCommon* dx_;
    static DescriptorPool* srvPool_;
};
