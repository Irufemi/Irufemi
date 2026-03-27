#pragma once
#include <d3d12.h>
#include <dxcapi.h> 
#include <wrl.h>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>
#include "../../Core/Type/BlendMode.h"

/**
 * @class PSOManager
 * @brief パイプラインステートオブジェクト（PSO）を管理・キャッシュするクラス
 * @details ブレンドモード、デプス書き込み設定、カリングモードなどの組み合わせに応じて
 *          PSO を生成・キャッシュし、描画時に適切なステートを提供します。
 *          同一の設定セットに対しては生成済みの PSO を再利用することで、
 *          実行時のステート切り替えコストを最適化します。
 */
class PSOManager {
public:
    /** @enum DepthWrite
     *  @brief 深度情報の扱い
     */
    enum class DepthWrite { 
        Enable,  ///< 深度書き込み有効
        Disable, ///< 深度テストのみ（書き込み無効）
        Off      ///< 深度テスト・書き込み共に無効
    };

    /** @enum CullMode
     *  @brief カリングモード
     */
    enum class CullMode { 
        Back,  ///< 背面カリング
        Front, ///< 前面カリング
        None   ///< カリングなし（両面描画）
    };

    /** @struct ShaderSet
     *  @brief 各シェーダステージのバイナリ（Blob）をまとめた構造体
     */
    struct ShaderSet {
        Microsoft::WRL::ComPtr<IDxcBlob> vsBlob; ///< 頂点シェーダ
        Microsoft::WRL::ComPtr<IDxcBlob> psBlob; ///< ピクセルシェーダ
        Microsoft::WRL::ComPtr<IDxcBlob> gsBlob; ///< ジオメトリシェーダ（任意）
    };

    /**
     * @brief 初期化処理
     * @details 各種描画コンポーネント用の基本シェーダセットを登録します。
     */
    void Initialize(
        ID3D12Device* device,
        ID3D12RootSignature* rootSig,
        const D3D12_INPUT_LAYOUT_DESC& inputLayout,
        DXGI_FORMAT rtvFormat,
        DXGI_FORMAT dsvFormat,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topology,
        ShaderSet objectShaders,         // 既存：Object3D.VS/PS など
        ShaderSet particleShaders = {}, // パーティクル専用 VS/PS
        ShaderSet spriteShaders = {},
        ShaderSet regionShaders = {},
        ShaderSet byGeometryShaderShaders = {},
        ShaderSet lineShaders = {},
        ShaderSet lineInstancedShaders = {},
        ShaderSet skinningShaders = {},
        ShaderSet skyboxShaders = {},
        ShaderSet gpuParticleShaders = {},
        ShaderSet voxelParticleShaders = {}
    );

    /** @name PSO取得（各種コンポーネント用） */
    ///@{
    /** @brief 通常オブジェクト用（静的メッシュ等） */
    ID3D12PipelineState* Get(BlendMode blend, DepthWrite depth, CullMode cull);
    /** @brief CPU制御パーティクル用 */
    ID3D12PipelineState* GetParticle(BlendMode blend, DepthWrite depth, CullMode cull);
    /** @brief 2Dスプライト用 */
    ID3D12PipelineState* GetSprite(BlendMode blend, DepthWrite depth, CullMode cull);
    /** @brief デバッグ・エディタ等の領域表示用 */
    ID3D12PipelineState* GetRegion(BlendMode b, DepthWrite d, CullMode c);
    /** @brief ジオメトリシェーダを使用する描画用 */
    ID3D12PipelineState* GetByGeometryShader(BlendMode blend, DepthWrite depth, CullMode cull);
    /** @brief 単一ライン描画用 */
    ID3D12PipelineState* GetLine(BlendMode blend, DepthWrite depth, CullMode cull);
    /** @brief インスタンシング対応ライン描画用 */
    ID3D12PipelineState* GetLineInstanced(BlendMode blend, DepthWrite depth, CullMode cull);
    /** @brief スキニング（ボーンアニメーション）対応オブジェクト用 */
    ID3D12PipelineState* GetSkinning(BlendMode blend, DepthWrite depth, CullMode cull);
    /** @brief スカイボックス用（カリングのみ指定） */
    ID3D12PipelineState* GetSkybox(CullMode cull);
    /** @brief GPUパーティクル描画用 */
    ID3D12PipelineState* GetGpuParticle(BlendMode blend, DepthWrite depth, CullMode cull);
    /** @brief Voxelパーティクル描画用 */
    ID3D12PipelineState* GetVoxelParticle(BlendMode blend, DepthWrite depth, CullMode cull);
    ///@}

    /** @name ポストプロセス・コピー */
    ///@{
    /** @brief 画面コピー用シェーダを設定 */
    void SetCopyImageShaders(const ShaderSet& shaders) { copyImageShaders_ = shaders; }
    /** @brief 画面コピー用 PSO を取得 */
    ID3D12PipelineState* GetCopyImage();
    ///@}

    /** @brief キャッシュされているすべての PSO を破棄する */
    void ClearCache();

private:
    using ComPtr = Microsoft::WRL::ComPtr<ID3D12PipelineState>;

    // デバイスおよびルートシグネチャ（RS/IL/RTV/DSV/Topology 等は固定）
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig_;
    D3D12_INPUT_LAYOUT_DESC inputLayout_{};
    
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements_;
    std::vector<std::string> semanticNames_;
    DXGI_FORMAT rtvFormat_{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };
    DXGI_FORMAT dsvFormat_{ DXGI_FORMAT_D24_UNORM_S8_UINT };
    D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };

    // 各用途ごとのベースシェーダ
    ShaderSet objectShaders_{};
    ShaderSet particleShaders_{};
    ShaderSet spriteShaders_{};
    ShaderSet blocksShaders_{};
    ShaderSet byGeometryShaderShaders_{};
    ShaderSet lineShaders_{};
    ShaderSet lineInstancedShaders_{};
    ShaderSet skinningShaders_{};
    ShaderSet skyboxShaders_{};
    ShaderSet gpuParticleShaders_{};
    ShaderSet voxelParticleShaders_{};
    ShaderSet copyImageShaders_{};

    /** @brief キャッシュキー構造体 */
    struct Key {
        uint64_t hash;
        bool operator==(const Key& o) const { return hash == o.hash; }
    };
    /** @brief キャッシュキーのハッシュ関数 */
    struct KeyHash { size_t operator()(const Key& k)const { return static_cast<size_t>(k.hash); } };

    std::unordered_map<Key, ComPtr, KeyHash> cache_; ///< PSO キャッシュ

    /** @name 内部生成ヘルパー */
    ///@{
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSO(
        const ShaderSet& shaders,
        const D3D12_BLEND_DESC& blendDesc,
        const D3D12_DEPTH_STENCIL_DESC& depthDesc,
        CullMode cull,
        bool useNullInputLayout = false) const;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSOWithTopology(
        const ShaderSet& shaders,
        const D3D12_BLEND_DESC& blendDesc,
        const D3D12_DEPTH_STENCIL_DESC& depthDesc,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topology,
        CullMode cull) const;

    /** @brief BlendMode から D3D12_BLEND_DESC を作成 */
    static D3D12_BLEND_DESC MakeBlend(BlendMode m);
    /** @brief DepthWrite から D3D12_DEPTH_STENCIL_DESC を作成 */
    static D3D12_DEPTH_STENCIL_DESC MakeDepth(DepthWrite w);

    /** @brief 設定セット（シェーダ、ブレンド、デプス、カリング）からハッシュ値を計算 */
    static uint64_t Hash(const ShaderSet& s, BlendMode b, DepthWrite d, CullMode c);
    ///@}
};