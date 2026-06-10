#pragma once

#include "../../../System/Core/IRenderable.h"
#include <d3d12.h>
#include <vector>
#include <string>
#include <memory>
#include <array>
#include <wrl.h>
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/Data/RenderPackets.h"

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

    void Initialize(const std::string& textureName = "resources/uvChecker.png");
    void Update();

    // インスタンスの追加
    void AddInstance(const Transform& transform, const Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f});
    void AddInstance(const Vector2& position, const Vector2& size, float rotation = 0.0f, const Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f}, const Vector2& anchor = {0.5f, 0.5f});
    
    void ClearInstances();

    void SyncBeforeDraw() override;
    void Draw() override;
    void Draw(bool isTopMost);

    // Getters
    Object2DResource* GetD3D12Resource() const { return baseResource_.get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const;
    UINT GetInstanceCount() const { return static_cast<UINT>(visibleInstanceCount_); }
    
    void SetTopMost(bool isTopMost) { isTopMost_ = isTopMost; }

    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetCameraManager(CameraManager* cm) { cameraManager_ = cm; }
    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    static void SetSrvAllocator(DescriptorPool* pool) { srvPool_ = pool; }

private:
    struct SpriteInstance {
        Transform transform;
        Vector4 color;
        Vector2 anchor; 
        Vector2 size;   
    };

    struct InstanceData {
        Matrix4x4 WVP;
        Vector4 color;
    };

    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);
    void BuildInstanceBuffer(bool force = false);
    void ApplyAnchorToVertices();

private:
    std::unique_ptr<Object2DResource> baseResource_ = nullptr;
    std::vector<SpriteInstance> instances_;

    uint32_t visibleInstanceCount_ = 0;
    bool instanceDirty_ = false;
    bool isTopMost_ = false;
    Vector2 textureSize_{0.0f, 0.0f};
    
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
