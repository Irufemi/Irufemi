#pragma once
#include <d3d12.h>
#include <dxcapi.h> 
#include <wrl.h>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>
#include "Engine/Core/Type/BlendMode.h"

/// <summary>
/// PSO をブレンド種別と深度書き込みの有無でキャッシュして返すマネージャ
/// ・Blend/DepthStencil は PSO に固定 → 描画直前に SetPipelineState で切替える
/// ・RootSignature / 入力レイアウト / RTV/DSV / トポロジ 等は IrufemiEngine の既存設定を流用
/// </summary>
/// 


class PSOManager {
public:

    enum class DepthWrite { Enable, Disable };

    // 追加: Cull 決定用(Depth と同様に扱う)
    enum class CullMode { Back, Front, None };

    struct ShaderSet {
        Microsoft::WRL::ComPtr<IDxcBlob> vsBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> psBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> gsBlob;
    };

    // 初期化(IrufemiEngine::Initialize から呼ぶ)
    void Initialize(
        ID3D12Device* device,
        ID3D12RootSignature* rootSig,
        const D3D12_INPUT_LAYOUT_DESC& inputLayout,
        DXGI_FORMAT rtvFormat,
        DXGI_FORMAT dsvFormat,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topology,
        ShaderSet objectShaders,         // 既存：Object3D.VS/PS など
        ShaderSet particleShaders = {}, // パーティクル専用 VS/PS(なければ空でOK)
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

    // 既存シェーダで取得(メッシュ/スプライト等)
    ID3D12PipelineState* Get(BlendMode blend, DepthWrite depth, CullMode cull);

    // パーティクル用シェーダで取得(未指定なら既存の objectShaders にフォールバック)
    ID3D12PipelineState* GetParticle(BlendMode blend, DepthWrite depth, CullMode cull);

    ID3D12PipelineState* GetSprite(BlendMode blend, DepthWrite depth, CullMode cull);

    // 
    ID3D12PipelineState* GetRegion(BlendMode b, DepthWrite d, CullMode c);

    // Geometry Shader を使う PSO を取得(未設定時は object にフォールバック)
    ID3D12PipelineState* GetByGeometryShader(BlendMode blend, DepthWrite depth, CullMode cull);

    ID3D12PipelineState* GetLine(BlendMode blend, DepthWrite depth, CullMode cull);

    ID3D12PipelineState* GetLineInstanced(BlendMode blend, DepthWrite depth, CullMode cull);

    ID3D12PipelineState* GetSkinning(BlendMode blend, DepthWrite depth, CullMode cull);

    ID3D12PipelineState* GetSkybox(CullMode cull);

    ID3D12PipelineState* GetGpuParticle(BlendMode blend, DepthWrite depth, CullMode cull);

    ID3D12PipelineState* GetVoxelParticle(BlendMode blend, DepthWrite depth, CullMode cull);

    // 追加: ポストプロセス用
    void SetCopyImageShaders(const ShaderSet& shaders) { copyImageShaders_ = shaders; }
    ID3D12PipelineState* GetCopyImage();

    void SetGrayscaleShaders(const ShaderSet& shaders) { grayscaleShaders_ = shaders; }
    ID3D12PipelineState* GetGrayscale();

    void SetSepiaShaders(const ShaderSet& shaders) { sepiaShaders_ = shaders; }
    ID3D12PipelineState* GetSepia();

    void SetVignetteShaders(const ShaderSet& shaders) { vignetteShaders_ = shaders; }
    ID3D12PipelineState* GetVignette();

    void ClearCache();

private:
    using ComPtr = Microsoft::WRL::ComPtr<ID3D12PipelineState>;

    // IrufemiEngine の固定情報(RS/IL/RTV/DSV/Topo)
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig_;
    D3D12_INPUT_LAYOUT_DESC inputLayout_{};
    // 要素配列を自前で所有(Initialize 時に深いコピー)
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements_;
    std::vector<std::string> semanticNames_;
    DXGI_FORMAT rtvFormat_{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };
    DXGI_FORMAT dsvFormat_{ DXGI_FORMAT_D24_UNORM_S8_UINT };
    D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };

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
    ShaderSet grayscaleShaders_{};
    ShaderSet sepiaShaders_{};
    ShaderSet vignetteShaders_{};

    struct Key {
        uint64_t hash;
        bool operator==(const Key& o) const { return hash == o.hash; }
    };
    struct KeyHash { size_t operator()(const Key& k)const { return static_cast<size_t>(k.hash); } };

    std::unordered_map<Key, ComPtr, KeyHash> cache_;

    // 内部ヘルパ
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSO(
        const ShaderSet& shaders,
        const D3D12_BLEND_DESC& blendDesc,
        const D3D12_DEPTH_STENCIL_DESC& depthDesc,
        CullMode cull,
        bool useNullInputLayout = false) const;

    // 追加：トポロジ指定版(ByGeometryShader 専用で使用)
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSOWithTopology(
        const ShaderSet& shaders,
        const D3D12_BLEND_DESC& blendDesc,
        const D3D12_DEPTH_STENCIL_DESC& depthDesc,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topology,
        CullMode cull) const;

    static D3D12_BLEND_DESC MakeBlend(BlendMode m);
    static D3D12_DEPTH_STENCIL_DESC MakeDepth(DepthWrite w);

    static uint64_t Hash(const ShaderSet& s, BlendMode b, DepthWrite d, CullMode c);
};