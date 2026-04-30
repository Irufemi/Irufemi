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
#include "../Graphics/Data/CameraForGPU.h"
#include "../Graphics/Data/DirectionalLight.h"
#include "../Graphics/DirectX/RenderTexture.h"
#include "../Graphics/DirectX/DirectXCommon.h" // kMaxFramesInFlight のために追加
#include "../Graphics/DirectX/RootSignatureConfig.h"
#include "../Core/Math/Vector4.h"
#include <vector>
#include <memory>
#include "IComputeTask.h"

class ShadowMap;

// 前方宣言
class TextureManager;
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
public:
    // --- Render Packets ---
    struct Standard3DPacket {
        const class Object3DResource* resource;
        const D3D12_VERTEX_BUFFER_VIEW* vertexBufferViewOverride;
        BlendMode blendMode;
        PSOManager::DepthWrite depthWrite;
        PSOManager::CullMode cullMode;
    };
    struct SpritePacket {
        const class Object2DResource* resource;
        BlendMode blendMode;
        PSOManager::DepthWrite depthWrite;
        PSOManager::CullMode cullMode;
    };
    struct ParticlePacket {
        const class ParticleResource* resource;
        uint32_t instanceCount;
        BlendMode blendMode;
        PSOManager::DepthWrite depthWrite;
        PSOManager::CullMode cullMode;
    };
    struct LinePacket {
        const class LineResource* resource;
        D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU;
        UINT instanceCount;
        BlendMode blendMode;
        PSOManager::DepthWrite depthWrite;
        PSOManager::CullMode cullMode;
    };
    struct GPUParticlePacket {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        D3D12_INDEX_BUFFER_VIEW ibv;
        uint32_t indexCount;
        D3D12_GPU_VIRTUAL_ADDRESS materialAddress;
        D3D12_GPU_VIRTUAL_ADDRESS perViewAddress;
        D3D12_GPU_VIRTUAL_ADDRESS emitterAddress;
        D3D12_GPU_DESCRIPTOR_HANDLE particleSrvHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle;
        uint32_t instanceCount;
        ID3D12Resource* particleResource;
        BlendMode blendMode;
        PSOManager::DepthWrite depthWrite;
        PSOManager::CullMode cullMode;
    };
    struct VoxelParticlePacket {
        uint32_t instanceCount;
        D3D12_VERTEX_BUFFER_VIEW vbv;
        D3D12_INDEX_BUFFER_VIEW ibv;
        uint32_t indexCount;
        D3D12_GPU_VIRTUAL_ADDRESS perViewAddress;
        D3D12_GPU_VIRTUAL_ADDRESS emitterAddress;
        D3D12_GPU_DESCRIPTOR_HANDLE particleDataHandle;
        ID3D12Resource* particleResource;
        ID3D12PipelineState* drawPSO;
    };
    struct SkyboxPacket {
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
        Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource;
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle;
        UINT indexCount;
    };
    struct RegionPacket {
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU;
        UINT indexCount;
        UINT instanceCount;
        BlendMode blendMode;
        PSOManager::DepthWrite depthWrite;
        PSOManager::CullMode cullMode;
    };
    struct ModelRegionPacket {
        const struct GpuMesh* gpuMesh;
        ID3D12Resource* materialResource;
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU;
        UINT instanceCount;
        BlendMode blendMode;
        PSOManager::DepthWrite depthWrite;
        PSOManager::CullMode cullMode;
    };

private:
    // --- Render Queues ---
    std::vector<Standard3DPacket> standard3DQueue_;
    std::vector<Standard3DPacket> ui3DQueue_;
    std::vector<SpritePacket> spriteQueue_;
    std::vector<ParticlePacket> particleQueue_;
    std::vector<LinePacket> lineQueue_;
    std::vector<GPUParticlePacket> gpuParticleQueue_;
    std::vector<VoxelParticlePacket> voxelParticleQueue_;
    std::vector<SkyboxPacket> skyboxQueue_;
    std::vector<RegionPacket> regionQueue_;
    std::vector<ModelRegionPacket> modelRegionQueue_;
    std::vector<std::function<void()>> postRenderQueue_;

public:
    // --- Execute Queues ---
    void ExecuteRenderQueues(class IrufemiEngine* engine);
    void ClearRenderQueues();
    void ClearAllQueues() {
        ClearRenderQueues();
        computeTasks_.clear();
    }

    DirectXCommon* dxCommon_ = nullptr;
    ID3D12GraphicsCommandList* commandList_ = nullptr; // コマンドリストをキャッシュ

    std::vector<IComputeTask*> computeTasks_;

    // 各フレームごとの動的リソース
    struct FrameResource {
        Microsoft::WRL::ComPtr<ID3D12Resource> frameResource;
        Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource;
        Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource;
        Microsoft::WRL::ComPtr<ID3D12Resource> areaLightResource;

        CameraForGPU* cameraData = nullptr;
        LightCommonData* lightCommonData = nullptr;

        D3D12_GPU_DESCRIPTOR_HANDLE lightSrvHandle{};
        uint32_t lightSrvBaseIndex = 0xFFFFFFFFu;
        
        // カメラやライト共通情報を格納するデータ
        struct FrameData {
            D3D12_GPU_VIRTUAL_ADDRESS camera;
            D3D12_GPU_VIRTUAL_ADDRESS lightCommon; // register b1
        } frameData{};
    };
    std::array<FrameResource, kMaxFramesInFlight> frameResources_;

    D3D12_GPU_DESCRIPTOR_HANDLE environmentMapHandle_{}; // 環境マップ用SRVハンドル

    // シャドウマップ・レンダーターゲット関連
    std::array<std::unique_ptr<ShadowMap>, kMaxFramesInFlight> shadowMaps_;
    bool isShadowPass_ = false;
    class RenderTexture* currentRenderTexture_ = nullptr;

public: //メンバ関数

    /** @name 初期化・終了処理 */
    ///@{
    DrawManager();
    ~DrawManager();

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
    void BeginShadowPass();
    void EndShadowPass();
    bool IsShadowPass() const { return isShadowPass_; }

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

    /** @name Computeタスク管理 */
    ///@{
    /**
     * @brief 今フレームで実行すべきComputeタスクを登録する
     * @param task 登録するタスク（GPUParticleSystem等）
     */
    void RegisterComputeTask(IComputeTask* task) {
        computeTasks_.push_back(task);
    }
    
    /**
     * @brief 登録された全Computeタスクを一括実行し、リストをクリアする
     */
    void ExecuteComputePasses();

    // カスタム描画コールバック用キュー
    void SubmitPostRender(std::function<void()> drawFunc) {
        postRenderQueue_.push_back(drawFunc);
    }
    ///@}
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
     * @brief キャッシュされたフレームデータを用いて現在のフレームバッファを同期する（ポーズなどでSetFrameDataが呼ばれなかった時用）
     */
    void SyncCachedFrameData();
    
private:
    CameraForGPU cachedCamera_{};
    DirectionalLight cachedDirectionalLight_{};
    std::vector<PointLight> cachedPointLights_;
    std::vector<SpotLight> cachedSpotLights_;
    std::vector<AreaLight> cachedAreaLights_;
    
    TextureManager* textureManager_ = nullptr; ///< 環境マップフォールバック等に使用するテクスチャマネージャー
    
public:

    /**
     * @brief テクスチャマネージャーのポインタを設定する
     * @param[in] textureManager 依存注入するテクスチャマネージャーへのポインタ
     */
    void SetTextureManager(TextureManager* textureManager) { textureManager_ = textureManager; }

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
    void SubmitParticle(const class ParticleResource* resource, uint32_t instanceCount);
    void DrawParticle(const ParticlePacket& packet);

    /**
     * @brief 矩形領域（Region）の描画
     */
    void SubmitModelRegion(const ModelRegionPacket& packet);
    void DrawModelRegion(const ModelRegionPacket& packet);

    /**
     * @brief 汎用的な領域描画（頂点バッファ・インデックスバッファ直接指定）
     */
    void SubmitRegion(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, const D3D12_GPU_DESCRIPTOR_HANDLE& textureHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& instancingSrvHandleGPU, const UINT& indexCount, const UINT& instanceCount);
    void DrawRegion(const RegionPacket& packet);

    /**
     * @brief インスタンス化された線の描画
     */
    void SubmitLineInstanced(const class LineResource* resource, const D3D12_GPU_DESCRIPTOR_HANDLE& instancingSrvHandleGPU, const UINT& instanceCount);
    void DrawLineInstanced(const LinePacket& packet);

    /**
     * @brief 標準的な3Dオブジェクトの描画 (Object3d.hlsl)
     * @param vertexBufferViewOverride スキニング等でVBVを差し替えたい場合に指定
     */
    void SubmitStandard3D(const class Object3DResource* resource, const D3D12_VERTEX_BUFFER_VIEW* vertexBufferViewOverride = nullptr);
    void SubmitUI3D(const class Object3DResource* resource, const D3D12_VERTEX_BUFFER_VIEW* vertexBufferViewOverride = nullptr);
    void DrawStandard3D(const Standard3DPacket& packet);

    /**
     * @brief 2Dオブジェクト（スプライト等）の標準描画 (Sprite.hlsl)
     */
    void SubmitSprite(const class Object2DResource* resource);
    void DrawSprite(const SpritePacket& packet);


    /**
     * @brief スカイボックスの描画
     */
    void SubmitSkybox(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource> materialResource, Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, const UINT& indexCount);
    void DrawSkybox(const SkyboxPacket& packet);

    /**
     * @brief GPUパーティクルのインスタンス描画 (GPUParticle.hlsl)
     */
    void SubmitGPUParticle(const D3D12_VERTEX_BUFFER_VIEW& vbv, const D3D12_INDEX_BUFFER_VIEW& ibv, uint32_t indexCount, D3D12_GPU_VIRTUAL_ADDRESS materialAddress, D3D12_GPU_VIRTUAL_ADDRESS perViewAddress, D3D12_GPU_VIRTUAL_ADDRESS emitterAddress, D3D12_GPU_DESCRIPTOR_HANDLE particleSrvHandle, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, uint32_t instanceCount, ID3D12Resource* particleResource);
    void DrawGPUParticle(const GPUParticlePacket& packet);
    
    // VoxelParticle 用の描画 (VoxelParticle.hlsl)
    void SubmitVoxelParticle(
        uint32_t instanceCount,
        const D3D12_VERTEX_BUFFER_VIEW& vbv,
        const D3D12_INDEX_BUFFER_VIEW& ibv,
        uint32_t indexCount,
        D3D12_GPU_VIRTUAL_ADDRESS perViewAddress,
        D3D12_GPU_VIRTUAL_ADDRESS emitterAddress,
        D3D12_GPU_DESCRIPTOR_HANDLE particleDataHandle,
        ID3D12Resource* particleResource,
        ID3D12PipelineState* drawPSO
    );
    void DrawVoxelParticle(const VoxelParticlePacket& packet);
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
    CameraForGPU* GetCameraData() const { return frameResources_[dxCommon_->GetFrameIndex()].cameraData; }
    DirectXCommon* GetDxCommon() const { return dxCommon_; }
    ShadowMap* GetShadowMap() const { return shadowMaps_[dxCommon_->GetFrameIndex()].get(); }
    ///@}
};