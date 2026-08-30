#pragma once
#include "Renderer/System/Core/BaseResource.h"
#include <vector>
#include "RHI/DirectX12/DirectXCommon.h"
#include <wrl.h>
#include <d3d12.h>
#include "Renderer/Data/VertexData.h"
#include "Renderer/Data/Material.h"
#include "Renderer/Data/TransformationMatrix.h"
#include "RHI/DirectX12/DynamicConstantBuffer.h"
#include "Core/System/ResourceHandle.h"
#include "Core/Math/Transform.h"

class Camera;

class Object3DResource : public BaseResource {
public:
    virtual ~Object3DResource();

    /**
     * @brief CreateResource を実行する。
     */
    void CreateResource() override;
    /**
     * @brief Map を実行する。
     */
    void Map() override;
    /**
     * @brief Unmap を実行する。
     */
    void Unmap() override;

    /**
     * @brief UpdateTransform を実行する。
     */
    void UpdateTransform(const Camera& camera);

    /**
     * @brief CustomPSO を設定する。
     * @param[in] pso 設定する CustomPSO の値
     */
    void SetCustomPSO(ID3D12PipelineState* pso) {
        customPSO_ = pso;
    }
    /**
     * @brief CustomPSO を取得する。
     * @return 取得された CustomPSO
     */
    ID3D12PipelineState* GetCustomPSO() const {
        return customPSO_;
    }

    /**
     * @brief CustomCBVAddress を設定する。
     * @param[in] addr 設定する CustomCBVAddress の値
     */
    void SetCustomCBVAddress(D3D12_GPU_VIRTUAL_ADDRESS addr) {
        customCBVAddress_ = addr;
    }
    /**
     * @brief CustomCBVAddress を取得する。
     * @return 取得された CustomCBVAddress
     */
    D3D12_GPU_VIRTUAL_ADDRESS GetCustomCBVAddress() const {
        return customCBVAddress_;
    }

public:
    // --- 頂点バッファ ---
    std::vector<VertexData> vertexDataList_{};
    VertexData* vertexData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    // --- インデックスバッファ ---
    std::vector<uint32_t> indexDataList_{};
    uint32_t* indexData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    uint32_t indexCount_ = 0;

    // --- マテリアル ---
    Irufemi::Transform uvTransform_{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    Material cpuMaterialData_{};
    /**
     * @brief MaterialData を取得する。
     * @return 取得された MaterialData
     */
    Material* GetMaterialData() {
        return &cpuMaterialData_;
    }

    uint32_t materialCbIndex_ = static_cast<uint32_t>(-1);

    // --- トランスフォーム ---
    Irufemi::Transform transform_{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    TransformationMatrix transformationMatrix_{};

    uint32_t transformCbIndex_ = static_cast<uint32_t>(-1);
    uint32_t* externalTransformCbIndex_ = nullptr;

    /**
     * @brief TransformationMatrix を取得する。
     * @return 取得された TransformationMatrix
     */
    const TransformationMatrix& GetTransformationMatrix() const {
        return transformationMatrix_;
    }

    // --- getters ---
    /**
     * @brief MaterialVAddress を取得する。
     * @return 取得された MaterialVAddress
     */
    D3D12_GPU_VIRTUAL_ADDRESS GetMaterialVAddress() const;
    /**
     * @brief TransformVAddress を取得する。
     * @return 取得された TransformVAddress
     */
    D3D12_GPU_VIRTUAL_ADDRESS GetTransformVAddress() const;

    /**
     * @brief SyncBeforeDraw を実行する。
     */
    void SyncBeforeDraw();

    // --- 外部リソースの借用 (StaticModelObject/AnimationModel等で共有するため) ---
    /**
     * @brief ExternalTransformCbIndex を設定する。
     * @param[in] externalCbIndex 設定する ExternalTransformCbIndex の値
     */
    void SetExternalTransformCbIndex(uint32_t* externalCbIndex) {
        externalTransformCbIndex_ = externalCbIndex;
    }

    // --- テクスチャ ---
    ResourceHandle textureHandle_;

    static class TextureManager* sTextureManager;

    bool isFirstUpdate_ = true;

    ID3D12PipelineState* customPSO_ = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS customCBVAddress_ = 0;
};
