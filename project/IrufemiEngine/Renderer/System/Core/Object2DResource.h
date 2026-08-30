#pragma once
#include "Core/Math/Transform.h"
#include "Core/System/ResourceHandle.h"
#include "RHI/DirectX12/DirectXCommon.h"
#include "RHI/DirectX12/DynamicConstantBuffer.h"
#include "Renderer/Data/Material.h"
#include "Renderer/Data/TransformationMatrix.h"
#include "Renderer/Data/VertexData.h"
#include "Renderer/System/Core/BaseResource.h"
#include <d3d12.h>
#include <vector>
#include <wrl.h>

class Camera;

class Object2DResource : public BaseResource {
public:
    virtual ~Object2DResource();

    /**
     * @brief 頂点・インデックス・定数バッファの生成またはサイズ調整を行う
     * @details 実行時、要求サイズが現在の Capacity(容量) に収まる場合は
     *          バッファの再生成をスキップし、パフォーマンス低下を防ぎます。
     *          定数バッファは未割り当ての場合のみ新規に Allocate します。
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

    ResourceHandle textureHandle_;

    static class TextureManager* sTextureManager;

    /**
     * @brief TransformVAddress を取得する。
     * @return 取得された TransformVAddress
     */
    D3D12_GPU_VIRTUAL_ADDRESS GetTransformVAddress() const;
    /**
     * @brief MaterialVAddress を取得する。
     * @return 取得された MaterialVAddress
     */
    D3D12_GPU_VIRTUAL_ADDRESS GetMaterialVAddress() const;

    // --- カスタム描画設定 ---
    ID3D12PipelineState* customPSO_ = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS customCBVAddress_ = 0;

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
     * @param[in] address 設定する CustomCBVAddress の値
     */
    void SetCustomCBVAddress(D3D12_GPU_VIRTUAL_ADDRESS address) {
        customCBVAddress_ = address;
    }
    /**
     * @brief CustomCBVAddress を取得する。
     * @return 取得された CustomCBVAddress
     */
    D3D12_GPU_VIRTUAL_ADDRESS GetCustomCBVAddress() const {
        return customCBVAddress_;
    }

    /**
     * @brief SyncBeforeDraw を実行する。
     */
    void SyncBeforeDraw();

private:
    uint32_t vertexCapacity_ = 0; //!< 現在確保されている頂点バッファの最大要素数
    uint32_t indexCapacity_ = 0;  //!< 現在確保されているインデックスバッファの最大要素数
};
