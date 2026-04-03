#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <array>
#include <wrl.h>
#include "../../Renderer/TransformationMatrix.h"
#include "../Graphics/Data/LightCommonData.h"
#include "../Graphics/Data/PointLight.h"
#include "../Graphics/Data/SpotLight.h"
#include "../Graphics/Data/AreaLight.h"
#include "../Graphics/DirectX/RenderTexture.h"
#include "../Graphics/DirectX/RootSignatureConfig.h"
#include "../Core/Math/Vector4.h"
#include <vector>
#include <memory>

// 前方宣言
class DirectXCommon;
class Sprite;
class TriangleClass;
class SphereClass;
class ObjClass;
class ParticleSystem;
class CylinderClass;
class TetraRegion;
class ModelRegion;
class SphereRegion;
class TetraRegion;
class SpriteRegion;
struct GpuMesh;
struct ManagedModel;
class Line2DClass;
class Line3DClass;
class Line3DRegion;
class CubeClass;
class Skybox;
struct SkinCluster;
struct GpuMaterial;

// 構造体を前方宣言
struct DirectionalLight;
struct CameraForGPU;

// 描画のCommandListを積む順番
// Viewport → RootSignature → Pipeline → Topology → Buffers → CBV → SRV → Draw

/**
 * @class DrawManager
 * @brief 描画コマンドの発行とパイプライン管理を担うクラス
 * @details 各レンダラーからの描画リクエストを受け取り、適切な順序でコマンドリストに積みます。
 *          ライト情報の管理や、RenderTexture を用いたポストプロセス実行の制御も行います。
 */
class DrawManager {
private:

    DirectXCommon* dxCommon_ = nullptr;
    ID3D12GraphicsCommandList* commandList_ = nullptr; // コマンドリストをキャッシュ

    // ライト関連のリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> areaLightResource_;

    // SRV ハンドルとインデックス
    D3D12_GPU_DESCRIPTOR_HANDLE pointLightSrvHandle_{};
    D3D12_GPU_DESCRIPTOR_HANDLE spotLightSrvHandle_{};
    D3D12_GPU_DESCRIPTOR_HANDLE areaLightSrvHandle_{};
    uint32_t pointLightSrvIndex_ = 0xFFFFFFFFu;
    uint32_t spotLightSrvIndex_ = 0xFFFFFFFFu;
    uint32_t areaLightSrvIndex_ = 0xFFFFFFFFu;

    // カメラやライト共通情報を格納するリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> frameResource_;
    struct FrameData {
        D3D12_GPU_VIRTUAL_ADDRESS camera;
        D3D12_GPU_VIRTUAL_ADDRESS lightCommon; // register b1
    } frameData_{};

    CameraForGPU* cameraData_ = nullptr;
    LightCommonData* lightCommonData_ = nullptr;

    D3D12_GPU_DESCRIPTOR_HANDLE environmentMapHandle_{}; // 環境マップ用SRVハンドル

public: //メンバ関数

    /** @name 初期化・終了処理 */
    ///@{
    void Initialize(DirectXCommon* dx);
    void Finalize();
    ///@}

    /** @name パイプライン・描画フロー制御 */
    ///@{
    /**
     * @brief パイプラインステートをバインドする
     * @param[in] pso バインドするパイプラインステート
     */
    void BindPSO(ID3D12PipelineState* pso);

    /**
     * @brief フレームの描画開始処理
     * @details レンダーターゲットのクリアとビューポートの設定を行います。
     */
    void PreDraw(
        std::array<float, 4> clearColor = { 0.1f, 0.25f, 0.5f, 1.0f },
        float clearDepth = 1.0f,
        uint8_t clearStencil = 0
    );

    /**
     * @brief フレームの描画終了処理
     * @details リソースバリアの変更とコマンドリストのクローズ準備を行います。
     */
    void PostDraw();

    /**
     * @brief フレーム共通のルートパラメータをバインドする
     * @details カメラ、ライト、各種管理用定数バッファを一括でレジスタに設定します。
     */
    void BindCommonParameters();
    ///@}

    /** @name レンダーターゲット・ポストプロセス操作 */
    ///@{
    /**
     * @brief 指定した RenderTexture への描画を開始する
     * @param[in] rt 出力先の RenderTexture
     * @param[in] clearColor 背景クリア色
     */
    void BeginRenderTexture(class RenderTexture* rt, const struct Vector4& clearColor);

    /**
     * @brief RenderTexture への描画を終了する
     * @param[in] rt 描画していた RenderTexture
     */
    void EndRenderTexture(class RenderTexture* rt);

    /**
     * @brief レンダーターゲットをバックバッファ（画面）に戻す
     * @param[in] useDepth 深度バッファを使用するかどうか
     */
    void SetRenderTargetToBackBuffer(bool useDepth = true);

    /**
     * @brief RenderTexture を全画面に描画（ポストプロセス用）
     * @param[in] renderTexture 描画元のテクスチャ
     * @param[in] pso 使用するポストプロセス用パイプラインステート
     * @param[in] cbvAddress 追加の定数バッファアドレス（オプション）
     * @param[in] depthSrvHandle 深度テクスチャのハンドル（オプション）
     */
    void DrawRenderTexture(class RenderTexture* renderTexture, ID3D12PipelineState* pso = nullptr, D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = 0, D3D12_GPU_DESCRIPTOR_HANDLE depthSrvHandle = { 0 });
    ///@}

    /** @name 共通データ設定 */
    ///@{
    /**
     * @brief フレーム単位の共通データを定数バッファに書き込む
     */
    void SetFrameData(const CameraForGPU& camera, const DirectionalLight& light, const std::vector<PointLight*>& pointLights, const std::vector<SpotLight*>& spotLights, const std::vector<AreaLight*>& areaLights);

    /**
     * @brief 環境マップを設定する
     * @param[in] envMapHandle 環境マップテクスチャのGPUハンドル
     */
    void SetEnvironmentMap(D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle);
    D3D12_GPU_DESCRIPTOR_HANDLE GetEnvironmentMap() const { return environmentMapHandle_; }
    ///@}

    /** @name 各種オブジェクト描画メソッド */
    ///@{
    /**
     * @brief パーティクルの描画（インスタンシング）
     */
    void DrawParticle(const class ParticleResource* resource, uint32_t instanceCount);

    /**
     * @brief 矩形領域（Region）の描画
     */
    void DrawModelRegion(ModelRegion* region);

    /**
     * @brief 汎用的な領域描画（頂点バッファ・インデックスバッファ直接指定）
     */
    void DrawRegion(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, const D3D12_GPU_DESCRIPTOR_HANDLE& textureHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& instancingSrvHandleGPU, const UINT& indexCount, const UINT& instanceCount);

    /**
     * @brief インスタンス化された線の描画
     */
    void DrawLineInstanced(const class LineResource* resource, const D3D12_GPU_DESCRIPTOR_HANDLE& instancingSrvHandleGPU, const UINT& instanceCount);

    /**
     * @brief 3Dオブジェクトの標準描画
     */
    void DrawObject3D(const class Object3DResource* resource);

    /**
     * @brief 2Dオブジェクト（スプライト等）の標準描画
     */
    void DrawObject2D(const class Object2DResource* resource);

    /**
     * @brief スカイボックスの描画
     */
    void DrawSkybox(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, const UINT& indexCount);

    /**
     * @brief GPUパーティクルの描画（直接バッファ指定）
     */
    void DrawParticleGPU(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_GPU_VIRTUAL_ADDRESS& material, const D3D12_GPU_VIRTUAL_ADDRESS& perView, const D3D12_GPU_DESCRIPTOR_HANDLE& textureHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& particleSrv, const UINT& instanceCount);
    ///@}

    /** @name コンピュートシェーダ（GPGPU）操作 */
    ///@{
    /**
     * @brief スキニング計算（Compute Shader）の実行
     */
    void DispatchSkinning(const SkinCluster& skinCluster, const ManagedModel* model, uint32_t numVertices);

    /**
     * @brief UAVバリアの実行（リソース競合の解決）
     */
    void ExecuteUAVBarrier(ID3D12Resource* resource);
    ///@}

    /** @name 状態取得・ユーティリティ */
    ///@{
    CameraForGPU* GetCameraData() const { return cameraData_; }
    DirectXCommon* GetDxCommon() const { return dxCommon_; }
    ///@}
};