#include "PSOManager.h"
#include <cstring>
#include <cassert>


// 軽量ハッシュ（キャッシュキー用）
static uint64_t FNV1a(const void* p, size_t n, uint64_t h = 1469598103934665603ull) {
    const uint8_t* b = (const uint8_t*)p; while (n--) { h ^= *b++; h *= 1099511628211ull; } return h;
}


void PSOManager::Initialize(
    ID3D12Device* device,
    ID3D12RootSignature* rootSig,
    const D3D12_INPUT_LAYOUT_DESC& inputLayout,
    DXGI_FORMAT rtvFormat,
    DXGI_FORMAT dsvFormat,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE topology,
    ShaderSet objectShaders,
    ShaderSet particleShaders,
    ShaderSet spriteShaders,
    ShaderSet regionShaders
)
{
    device_ = device;
    rootSig_ = rootSig;
    // ★ディープコピー：要素配列を所有し、inputLayout_ には自前のポインタを設定
    inputElements_.assign(inputLayout.pInputElementDescs,
        inputLayout.pInputElementDescs + inputLayout.NumElements);
    inputLayout_.pInputElementDescs = inputElements_.data();
    inputLayout_.NumElements = static_cast<UINT>(inputElements_.size());
    rtvFormat_ = rtvFormat; // 既存の RTV 形式
    dsvFormat_ = dsvFormat; // 既存の DSV 形式
    topology_ = topology; // 三角形トポロジ固定（既存）
    objectShaders_ = objectShaders; // 既存 VS/PS（Object3D）
    particleShaders_ = particleShaders; // パーティクル VS/PS（あれば）
    spriteShaders_ = spriteShaders;
    blocksShaders_ = regionShaders;


    cache_.clear();
}


ID3D12PipelineState* PSOManager::Get(BlendMode blend, DepthWrite depth)
{
    Key key{ Hash(objectShaders_, blend, depth) };
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second.Get();


    D3D12_BLEND_DESC bd = MakeBlend(blend);
    D3D12_DEPTH_STENCIL_DESC dd = MakeDepth(depth);
    auto p = CreatePSO(objectShaders_, bd, dd);
    if (!p) { return nullptr; }
    cache_[key] = p;
    return p.Get();
}


ID3D12PipelineState* PSOManager::GetParticle(BlendMode blend, DepthWrite depth)
{
    const bool hasParticleVS = (particleShaders_.vsBlob && particleShaders_.vsBlob->GetBufferPointer());
    const bool hasParticlePS = (particleShaders_.psBlob && particleShaders_.psBlob->GetBufferPointer());
    const ShaderSet& set = (hasParticleVS && hasParticlePS) ? particleShaders_ : objectShaders_;


    Key key{ Hash(set, blend, depth) };
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second.Get();


    D3D12_BLEND_DESC bd = MakeBlend(blend);
    D3D12_DEPTH_STENCIL_DESC dd = MakeDepth(depth);
    auto p = CreatePSO(set, bd, dd);
    if (!p) { return nullptr; }            // ★追加
    cache_[key] = p;
    return p.Get();
}

ID3D12PipelineState* PSOManager::GetSprite(BlendMode blend, DepthWrite depth) {
    // Sprite用シェーダが未設定なら Object 用にフォールバック
    const ShaderSet& shaders = (spriteShaders_.vsBlob && spriteShaders_.psBlob)
        ? spriteShaders_
        : objectShaders_;

    // キャッシュキー（Sprite識別のために XOR で種を追加）
    constexpr uint64_t kSpriteTag = 0x535052544B4559ull; // "SPR TKEY"
    Key key{ static_cast<uint64_t>(Hash(shaders, blend, depth) ^ kSpriteTag) };

    auto it = cache_.find(key);
    if (it != cache_.end()) { return it->second.Get(); }

    // PSO 構築（Rasterizer.Cull=None に固定）
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSig_.Get();
    desc.VS = { shaders.vsBlob ? shaders.vsBlob->GetBufferPointer() : nullptr,
                shaders.vsBlob ? shaders.vsBlob->GetBufferSize() : 0 };
    desc.PS = { shaders.psBlob ? shaders.psBlob->GetBufferPointer() : nullptr,
                shaders.psBlob ? shaders.psBlob->GetBufferSize() : 0 };
    desc.InputLayout = inputLayout_;
    desc.PrimitiveTopologyType = topology_;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = rtvFormat_;
    desc.DSVFormat = dsvFormat_;
    desc.SampleMask = UINT_MAX;
    desc.SampleDesc.Count = 1;

    // Blend / Depth
    desc.BlendState = MakeBlend(blend);
    desc.DepthStencilState = MakeDepth(depth);

    // Rasterizer（ここで CullMode = NONE）
    D3D12_RASTERIZER_DESC rast{};
    rast.FillMode = D3D12_FILL_MODE_SOLID;
    rast.CullMode = D3D12_CULL_MODE_NONE; // ← 要件
    rast.FrontCounterClockwise = FALSE;
    rast.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rast.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rast.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rast.DepthClipEnable = TRUE;
    rast.MultisampleEnable = FALSE;
    rast.AntialiasedLineEnable = FALSE;
    rast.ForcedSampleCount = 0;
    rast.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    desc.RasterizerState = rast;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
    assert(SUCCEEDED(hr));
    cache_.emplace(key, pso);
    return pso.Get();
}

ID3D12PipelineState* PSOManager::GetRegion(BlendMode b, DepthWrite d)
{
    // 既存のキャッシュ Key 生成を流用（Hash(blocksShaders_, b, d)）
    Key k{ Hash(blocksShaders_, b, d) };
    auto it = cache_.find(k);
    if (it != cache_.end()) return it->second.Get();

    auto blend = MakeBlend(b);
    auto depth = MakeDepth(d);
    auto pso = CreatePSO(blocksShaders_, blend, depth);
    cache_[k] = pso;
    return pso.Get();
}

void PSOManager::ClearCache() { cache_.clear(); }

// PSOManager.cpp に追加
Microsoft::WRL::ComPtr<ID3D12PipelineState> PSOManager::CreatePSO(
    const ShaderSet& shaders,
    const D3D12_BLEND_DESC& blendDesc,
    const D3D12_DEPTH_STENCIL_DESC& depthDesc) const
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSig_.Get();
    desc.InputLayout = inputLayout_;
    desc.VS = { shaders.vsBlob->GetBufferPointer(), shaders.vsBlob->GetBufferSize() };
    desc.PS = { shaders.psBlob->GetBufferPointer(), shaders.psBlob->GetBufferSize() };
    desc.BlendState = blendDesc;

    // ラスタライザ（既存エンジンのデフォルトに合わせる）
    D3D12_RASTERIZER_DESC rs{};
    rs.CullMode = D3D12_CULL_MODE_BACK;
    rs.FillMode = D3D12_FILL_MODE_SOLID;
    desc.RasterizerState = rs;

    desc.DepthStencilState = depthDesc;
    desc.DSVFormat = dsvFormat_;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = rtvFormat_;
    desc.PrimitiveTopologyType = topology_;
    desc.SampleDesc.Count = 1;
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
    assert(SUCCEEDED(hr) && "CreateGraphicsPipelineState failed");
    if (FAILED(hr)) { return nullptr; }
    return pso;
}

// Multiply : out = src * dst
// Screen : out = src * (1 - dst) + dst * 1
D3D12_BLEND_DESC PSOManager::MakeBlend(BlendMode m)
{
    D3D12_BLEND_DESC d{}; auto& rt = d.RenderTarget[0];
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // すべての色要素を書き込む（既存コメント踏襲）


    switch (m) {
    case BlendMode::kBlendModeNone:
        // BlendEnable = FALSE（ブレンドなし）
        break;


    case BlendMode::kBlendModeNormal: // Normal
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        rt.BlendOp = D3D12_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D12_BLEND_ONE; // αの設定：基本は固定（既存コメントと同じ）
        rt.DestBlendAlpha = D3D12_BLEND_ZERO;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;


    case BlendMode::kBlendModeAdd: // Add
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt.DestBlend = D3D12_BLEND_ONE;
        rt.BlendOp = D3D12_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_ONE;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;


    case BlendMode::kBlendModeSubtract: // Subtract（RGBは REV_SUBTRACT）
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D12_BLEND_ONE; // RGB: 1 - 1 の係数で REV_SUBTRACT
        rt.DestBlend = D3D12_BLEND_ONE;
        rt.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // dst - src
        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_ONE;
        rt.BlendOpAlpha = D3D12_BLEND_OP_REV_SUBTRACT;
        break;


    case BlendMode::kBlendModeMultiply: // Multiply（src * dst）
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D12_BLEND_DEST_COLOR;
        rt.DestBlend = D3D12_BLEND_ZERO;
        rt.BlendOp = D3D12_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_ZERO;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;


    case BlendMode::kBlendModeScreen: // Screen（src*(1-dst)+dst）
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        rt.DestBlend = D3D12_BLEND_ONE;
        rt.BlendOp = D3D12_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_ONE;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;


    default: break;
    }
    return d;
}

D3D12_DEPTH_STENCIL_DESC PSOManager::MakeDepth(DepthWrite w)
{
    D3D12_DEPTH_STENCIL_DESC d{};
    d.DepthEnable = TRUE;                                      // Depth の機能を有効化する（既存コメント踏襲）
    d.DepthWriteMask = (w == DepthWrite::Enable) ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO; // 書き込みします/しません
    d.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;            // つまり、近ければ描画される（既存コメント踏襲）
    return d;
}

uint64_t PSOManager::Hash(const ShaderSet& s, BlendMode b, DepthWrite d)
{
    uint64_t h = 0;
    // VS
    const void* vsPtr = (s.vsBlob ? s.vsBlob->GetBufferPointer() : nullptr);
    const size_t vsLen = (s.vsBlob ? s.vsBlob->GetBufferSize() : 0);
    h = FNV1a(&vsPtr, sizeof(vsPtr), h);
    h = FNV1a(&vsLen, sizeof(vsLen), h);
    // PS
    const void* psPtr = (s.psBlob ? s.psBlob->GetBufferPointer() : nullptr);
    const size_t psLen = (s.psBlob ? s.psBlob->GetBufferSize() : 0);
    h = FNV1a(&psPtr, sizeof(psPtr), h);
    h = FNV1a(&psLen, sizeof(psLen), h);
    h = FNV1a(&b, sizeof(b), h);
    h = FNV1a(&d, sizeof(d), h);
    return h;
}
