#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <string>
#include "Renderer/Data/Material.h"
#include "Renderer/Data/VertexData.h"
#include "Renderer/System/Core/MultiBufferSyncState.h"
#include "Core/Math/Transform.h"
#include "Core/Math/Matrix4x4.h"
#include "Core/Math/Vector4.h"
#include <vector>
#include <array>
#include "RHI/DirectX12/DirectXCommon.h"
#include "RHI/DirectX12/ConstantBuffer.h"
#include "Core/System/ResourceHandle.h"

// 前方宣言
class Camera;
class IrufemiEngine;

#include "Renderer/System/Core/IRenderable.h"

/**
 * @class Skybox
 * @brief スカイボックスの描画を管理するクラス
 */
class Skybox : public IRenderable, public MultiBufferSyncState
{
public:
public: // メンバ関数
    // コンストラクタ
    Skybox();
    // デストラクタ
    ~Skybox();
    // 初期化
    /**
     * @brief Initialize を実行する。
     */
    void Initialize(const std::string& textureName = "resources/rostock_laage_airport_4k.dds");
    // 更新
    /**
     * @brief Update を実行する。
     */
    void Update();
    /**
     * @brief SyncBeforeDraw を実行する。
     */
    void SyncBeforeDraw() override;
    /**
     * @brief Draw を実行する。
     */
    void Draw() override;
    // デバッグ
    /**
     * @brief Debug を実行する。
     */
    void Debug();
public: // メンバ関数(セッター/ゲッター)
    // engineセッター
    /**
     * @brief Engine を設定する。
     * @param[in] engine 設定する Engine の値
     */
    static void SetEngine(IrufemiEngine* engine) { engine_ = engine; }
    // ID3D12Resource関連ゲッター
    /**
     * @brief VertexBufferView を取得する。
     * @return 取得された VertexBufferView
     */
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBufferView_; }
    /**
     * @brief IndexBufferView を取得する。
     * @return 取得された IndexBufferView
     */
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBufferView_; }
    /**
     * @brief TextureHandle を取得する。
     * @return 取得された TextureHandle
     */
    ResourceHandle GetTextureHandle() const { return textureHandle_; }
    // indexのサイズ取得
    /**
     * @brief IndexSize を取得する。
     * @return 取得された IndexSize
     */
    UINT GetIndexSize() const { return static_cast<UINT>(indexDataList_.size()); }
private: // メンバ関数(内部ヘルパ)
    // ID3D12Resourceの生成
    /**
     * @brief CreateResource を実行する。
     */
    void CreateResource();
    // ID3D12ResourceのMap
    /**
     * @brief MapResource を実行する。
     */
    void MapResource();
    // Id3D12ResourceのUnMap
    /**
     * @brief UnMapResource を実行する。
     */
    void UnMapResource();


private: // メンバ変数(resource)
    /// vertex
    std::vector<VertexData> vertexDataList_{};
    VertexData* vertexData_ = nullptr;
    //頂点データバッファ
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;

    /// index
    std::vector<uint32_t> indexDataList_{};
    uint32_t* indexData_ = nullptr;
    //頂点インデックスバッファ
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;

    /// Irufemi::Transform
    // transform(scale,rotate,translate)
    Irufemi::Transform transform_ = {
        {500.0f,500.0f,500.0f},   //scale
        {0.0f,0.0f,0.0f},   //rotate
        {0.0f,0.0f,0.0f}    //translate
    };
    struct SkyboxTransformationMatrix {
        Irufemi::Matrix4x4 WVP;
        Irufemi::Matrix4x4 World;
        Irufemi::Matrix4x4 WorldInverseTranspose;
    };
    SkyboxTransformationMatrix transformationMatrix_{};
    ConstantBuffer<SkyboxTransformationMatrix> transformationBuffer_;

    // Material
    struct SkyboxMaterial {
        Irufemi::Vector4 color;
        float intensity;
        uint32_t textureIndex; // [Bindless]
        uint32_t padding[2];
    };
    ConstantBuffer<SkyboxMaterial> materialBuffer_;

    // texture
    ResourceHandle textureHandle_;
    int selectedTextureIndex_ = 0;

    // カメラ(ポインタ参照)

    // engine(ポインタ参照)
    static IrufemiEngine* engine_;

    // 行列更新の最適化用
    bool isDirty_ = true;
    Irufemi::Matrix4x4 lastViewMatrix_ = {};
    Irufemi::Matrix4x4 lastProjectionMatrix_ = {};

};


