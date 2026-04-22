#include "DirectXCommon.h"

#include "Resource/Texture/TextureUtility.h"
#include <string>
#include <cassert>
#include <vector>
#include <comdef.h>

#include "../../Core/Utility/Log.h"
#include "../../Core/Utility/StringUtility.h"
#include "../../../../externals/DirectXTex/d3dx12.h"
#include <algorithm>

void DirectXCommon::Finalize() {


    // GPU同期 (全フレームの完了を待機)
    if (commandQueue_ && fence_) {
        for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
            uint64_t fv = IncrementGlobalFence();
            commandQueue_->Signal(fence_.Get(), fv);
            if (fence_->GetCompletedValue() < fv) {
                fence_->SetEventOnCompletion(fv, fenceEvent_);
                WaitForSingleObject(fenceEvent_, INFINITE);
            }
        }
    }

    // フェンスイベント
    if (fenceEvent_) {
        CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
    }

    // PSO キャッシュを解放(PSO/RSの参照を切る)
    if (psoManager_) {
        psoManager_->ClearCache();
        psoManager_.reset();
    }

    // D3D12解放順: PSO/RootSig→DSV/RTV/SRV→バッファ→コマンド系→フェンス→SwapChain→Device
    voxelParticleUpdatePSO_.Reset();
    voxelParticleEmitPSO_.Reset();
    gpuParticleUpdatePSO_.Reset();
    gpuParticleInitializePSO_.Reset();
    skinningComputePSO_.Reset();
    computeRootSignature_.Reset();
    rootSignature_.Reset();
    depthStencilResource_.Reset();
    rtvDescriptorHeap_.Reset();
    srvPool_.reset();
    dsvDescriptorHeap_.Reset();
    swapChainResources_[0].Reset();
    swapChainResources_[1].Reset();
    uploadCommandList_.Reset();
    uploadCommandAllocator_.Reset();
    uploadFence_.Reset();
    commandList_.Reset();
    for (auto& allocator : commandAllocators_) {
        allocator.Reset();
    }
    commandQueue_.Reset();
    fence_.Reset();
    swapChain_.Reset();
    device_.Reset();

#if defined(_DEBUG) || defined(DEVELOPMENT)
    debugController_.Reset();
#endif

    if (hwnd_) {
        hwnd_ = nullptr;
    }
}

void DirectXCommon::Initialize(HWND hwnd, int32_t w, int32_t h) {
    hwnd_ = hwnd;
    clientWidth_ = w;
    clientHeight_ = h;

    // 制御用クラスの生成
    fpsController_ = std::make_unique<FrameRateController>();
    shaderCompiler_ = std::make_unique<ShaderCompiler>();

    InitializeFixFPS();
    shaderCompiler_->Initialize();

    EnableDebugLayer();
    InitializeDXGI();
    CreateDevice();
    SetInfoQueue();
    CreateCommandObjects();
    CreateSwapChain();
    
    descriptorSizeRTV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptorSizeDSV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    CreateDescriptorHeaps();
    InitializeRenderTargets();
    CreateDepthStencil();
    CreateFence();
    CreateRootSignatures();
    CreatePSOs();
}

void DirectXCommon::EnableDebugLayer() {
#if defined(_DEBUG) || defined(DEVELOPMENT)
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController_.GetAddressOf())))) {
        //デバッグレイヤーを有効化する
        debugController_->EnableDebugLayer();
        //さらにGPU側でもチェックを行うようにする
        debugController_->SetEnableGPUBasedValidation(TRUE);
    }
#endif
}

void DirectXCommon::InitializeDXGI() {
    //DXGIFactoryの生成
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory_.GetAddressOf()));
    assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateDevice() {
    ///使用するアダプタ(GPU)を決定する
    Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
    for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(useAdapter.GetAddressOf())) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC3 adapterDesc{};
        HRESULT hr = useAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));
        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            // std::format が環境によって不安定な可能性があるため、wstringstream等で代用するか、ワイド文字版が正しく動作することを確認
            std::wstring adapterName = adapterDesc.Description;
            Log::OutPutLog(log_->GetLogStream(), "use Adapter:" + ConvertString(adapterName) + "\n");
            break;
        }
        useAdapter = nullptr;
    }
    assert(useAdapter != nullptr);

    ///D3D12Deviceの生成
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
    };
    const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
    for (size_t i = 0; i < _countof(featureLevels); ++i) {
        HRESULT hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(device_.GetAddressOf()));
        if (SUCCEEDED(hr)) {
            Log::OutPutLog(log_->GetLogStream(), std::string("FeatureLevel : ") + featureLevelStrings[i] + "\n");
            break;
        }
    }
    assert(device_ != nullptr);
    Log::OutPutLog(log_->GetLogStream(), "Complete create D3D12Device!!!\n");

    if (useAdapter) { useAdapter.Reset(); }
}


void DirectXCommon::SetInfoQueue() {
#if defined(_DEBUG) || defined(DEVELOPMENT)
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

        D3D12_MESSAGE_ID denyIds[] = {
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
        };
        D3D12_MESSAGE_SEVERITY severties[] = { D3D12_MESSAGE_SEVERITY_INFO };
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severties);
        filter.DenyList.pSeverityList = severties;
        infoQueue->PushStorageFilter(&filter);
        infoQueue.Reset();
    }
#endif
}

void DirectXCommon::CreateCommandObjects() {
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    HRESULT hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue_.GetAddressOf()));
    assert(SUCCEEDED(hr));

    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocators_[i].GetAddressOf()));
        assert(SUCCEEDED(hr));
    }

    hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators_[0].Get(), nullptr, IID_PPV_ARGS(commandList_.GetAddressOf()));
    assert(SUCCEEDED(hr));
    commandList_->Close();

    // 転送専用コマンド系の生成
    hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(uploadCommandAllocator_.GetAddressOf()));
    assert(SUCCEEDED(hr));
    hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, uploadCommandAllocator_.Get(), nullptr, IID_PPV_ARGS(uploadCommandList_.GetAddressOf()));
    assert(SUCCEEDED(hr));
    uploadCommandList_->Close();

    hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(uploadFence_.GetAddressOf()));
    assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateSwapChain() {
    swapChainDesc_.Width = clientWidth_;
    swapChainDesc_.Height = clientHeight_;
    swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc_.SampleDesc.Count = 1;
    swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc_.BufferCount = 2;
    swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_.Get(), hwnd_, &swapChainDesc_, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
    assert(SUCCEEDED(hr));

    if (dxgiFactory_) { dxgiFactory_.Reset(); }

    for (uint32_t i = 0; i < 2; ++i) {
        hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(swapChainResources_[i].GetAddressOf()));
        assert(SUCCEEDED(hr));
    }
}

void DirectXCommon::CreateDescriptorHeaps() {
    rtvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 128, false);
    nextRtvIndex_ = 4; // 0, 1 は SwapChain 用、2, 3 は ImGui 用に予約

    srvPool_ = std::make_unique<DescriptorPool>();
    srvPool_->Initialize(device_.Get());

    dsvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 16, false);
    nextDsvIndex_ = 1; // 0 はメインの深度バッファ
}

void DirectXCommon::InitializeRenderTargets() {
    rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    for (uint32_t i = 0; i < 2; ++i) {
        rtvHandles_[i].ptr = rtvStartHandle.ptr + (i * descriptorSizeRTV_);
        device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc_, rtvHandles_[i]);
    }

    // ImGui用 RTV
    D3D12_RENDER_TARGET_VIEW_DESC imGuiRtvDesc = rtvDesc_;
    imGuiRtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    for (uint32_t i = 0; i < 2; ++i) {
        rtvHandles_[i + 2].ptr = rtvHandles_[1].ptr + ((i + 1) * descriptorSizeRTV_);
        device_->CreateRenderTargetView(swapChainResources_[i].Get(), &imGuiRtvDesc, rtvHandles_[i + 2]);
    }
}

void DirectXCommon::CreateDepthStencil() {
    depthStencilResource_ = CreateDepthStencilTextureResource(device_.Get(), clientWidth_, clientHeight_);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::CreateFence() {
    HRESULT hr = device_->CreateFence(fenceValues_[0], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf()));
    assert(SUCCEEDED(hr));

    fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent_ != nullptr);
}

void DirectXCommon::CreateRootSignatures() {
    // --- 通常描画用 RootSignature ---
    {
        // --- ディスクリプタレンジの定義 ---

        D3D12_DESCRIPTOR_RANGE rangeTexture[1] = {};
        rangeTexture[0].BaseShaderRegister = 0; // t0
        rangeTexture[0].NumDescriptors = 1;
        rangeTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        rangeTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE rangeInstancing[1] = {};
        rangeInstancing[0].BaseShaderRegister = 0; // t0
        rangeInstancing[0].NumDescriptors = 1;
        rangeInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        rangeInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE rangeEnv[1] = {};
        rangeEnv[0].BaseShaderRegister = 1; // t1
        rangeEnv[0].NumDescriptors = 1;
        rangeEnv[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        rangeEnv[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE rangeLine[1] = {};
        rangeLine[0].BaseShaderRegister = 1; // t1
        rangeLine[0].NumDescriptors = 1;
        rangeLine[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        rangeLine[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // ライトSRVテーブル (t2, t3, t4 を一括バインド)
        D3D12_DESCRIPTOR_RANGE rangeLights[1] = {};
        rangeLights[0].BaseShaderRegister = 2; // t2 から開始
        rangeLights[0].NumDescriptors = 3;     // t2, t3, t4 の3つ分
        rangeLights[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        rangeLights[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // シャドウマップ (t5)
        D3D12_DESCRIPTOR_RANGE rangeShadow[1] = {};
        rangeShadow[0].BaseShaderRegister = 5; // t5
        rangeShadow[0].NumDescriptors = 1;
        rangeShadow[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        rangeShadow[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // --- ルートパラメータの定義 ---
        D3D12_ROOT_PARAMETER rootParameters[11] = {};

        // Slot 0: Material (b0, PS)
        rootParameters[(UINT)RootSlot::Material].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[(UINT)RootSlot::Material].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[(UINT)RootSlot::Material].Descriptor.ShaderRegister = 0;

        // Slot 1: Transform (b0, VS)
        rootParameters[(UINT)RootSlot::Transform].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[(UINT)RootSlot::Transform].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[(UINT)RootSlot::Transform].Descriptor.ShaderRegister = 0;

        // Slot 2: Texture (t0, PS)
        rootParameters[(UINT)RootSlot::Texture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[(UINT)RootSlot::Texture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[(UINT)RootSlot::Texture].DescriptorTable.pDescriptorRanges = rangeTexture;
        rootParameters[(UINT)RootSlot::Texture].DescriptorTable.NumDescriptorRanges = 1;

        // Slot 3: LightCommon (b1, ALL)
        rootParameters[(UINT)RootSlot::LightCommon].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[(UINT)RootSlot::LightCommon].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[(UINT)RootSlot::LightCommon].Descriptor.ShaderRegister = 1;

        // Slot 4: Instancing (t0, VS)
        rootParameters[(UINT)RootSlot::Instancing].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[(UINT)RootSlot::Instancing].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[(UINT)RootSlot::Instancing].DescriptorTable.pDescriptorRanges = rangeInstancing;
        rootParameters[(UINT)RootSlot::Instancing].DescriptorTable.NumDescriptorRanges = 1;

        // Slot 5: Camera (b2, ALL)
        rootParameters[(UINT)RootSlot::Camera].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[(UINT)RootSlot::Camera].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[(UINT)RootSlot::Camera].Descriptor.ShaderRegister = 2;

        // Slot 6: Lights (t2-t4, PS)
        rootParameters[(UINT)RootSlot::Lights].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[(UINT)RootSlot::Lights].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[(UINT)RootSlot::Lights].DescriptorTable.pDescriptorRanges = rangeLights;
        rootParameters[(UINT)RootSlot::Lights].DescriptorTable.NumDescriptorRanges = 1;

        // Slot 7: Special (b6, ALL)
        rootParameters[(UINT)RootSlot::Special].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[(UINT)RootSlot::Special].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[(UINT)RootSlot::Special].Descriptor.ShaderRegister = 6;

        // Slot 8: EnvMap (t1, PS)
        rootParameters[(UINT)RootSlot::EnvMap].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[(UINT)RootSlot::EnvMap].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[(UINT)RootSlot::EnvMap].DescriptorTable.pDescriptorRanges = rangeEnv;
        rootParameters[(UINT)RootSlot::EnvMap].DescriptorTable.NumDescriptorRanges = 1;

        // Slot 9: LineInstancing (t1, VS)
        rootParameters[(UINT)RootSlot::LineInstancing].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[(UINT)RootSlot::LineInstancing].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[(UINT)RootSlot::LineInstancing].DescriptorTable.pDescriptorRanges = rangeLine;
        rootParameters[(UINT)RootSlot::LineInstancing].DescriptorTable.NumDescriptorRanges = 1;

        // Slot 10: ShadowMap (t5, PS)
        rootParameters[(UINT)RootSlot::ShadowMap].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[(UINT)RootSlot::ShadowMap].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[(UINT)RootSlot::ShadowMap].DescriptorTable.pDescriptorRanges = rangeShadow;
        rootParameters[(UINT)RootSlot::ShadowMap].DescriptorTable.NumDescriptorRanges = 1;

        D3D12_STATIC_SAMPLER_DESC staticSamplers[4] = {};
        staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[0].ShaderRegister = 0;
        staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[1].ShaderRegister = 1;
        staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        staticSamplers[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        staticSamplers[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticSamplers[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticSamplers[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticSamplers[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        staticSamplers[2].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        staticSamplers[2].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[2].ShaderRegister = 2; // s2
        staticSamplers[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        staticSamplers[3].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSamplers[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[3].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSamplers[3].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[3].ShaderRegister = 3; // s3
        staticSamplers[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rsDesc{};
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        rsDesc.pParameters = rootParameters;
        rsDesc.NumParameters = _countof(rootParameters);
        rsDesc.pStaticSamplers = staticSamplers;
        rsDesc.NumStaticSamplers = _countof(staticSamplers);

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
        HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, signatureBlob.GetAddressOf(), errorBlob.GetAddressOf());
        if (FAILED(hr)) {
            Log::OutPutLog(log_->GetLogStream(), reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
            assert(false);
        }
        hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(rootSignature_.GetAddressOf()));
        assert(SUCCEEDED(hr));
    }

    // --- Compute Shader用 RootSignature ---
    {
        D3D12_DESCRIPTOR_RANGE srvRanges[3];
        srvRanges[0] = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }; // t0
        srvRanges[1] = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }; // t1
        srvRanges[2] = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }; // t2

        D3D12_DESCRIPTOR_RANGE uavRanges[3];
        uavRanges[0] = { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }; // u0
        uavRanges[1] = { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }; // u1
        uavRanges[2] = { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }; // u2

        D3D12_ROOT_PARAMETER computeRootParameters[8] = {};
        computeRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        computeRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        computeRootParameters[0].DescriptorTable.pDescriptorRanges = &srvRanges[0];
        computeRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;

        computeRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        computeRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        computeRootParameters[1].DescriptorTable.pDescriptorRanges = &srvRanges[1];
        computeRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;

        computeRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        computeRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        computeRootParameters[2].DescriptorTable.pDescriptorRanges = &srvRanges[2];
        computeRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

        computeRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        computeRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        computeRootParameters[3].DescriptorTable.pDescriptorRanges = &uavRanges[0];
        computeRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;

        computeRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        computeRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        computeRootParameters[4].Descriptor.ShaderRegister = 0; // b0

        computeRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        computeRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        computeRootParameters[5].Descriptor.ShaderRegister = 1; // b1

        computeRootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        computeRootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        computeRootParameters[6].DescriptorTable.pDescriptorRanges = &uavRanges[1];
        computeRootParameters[6].DescriptorTable.NumDescriptorRanges = 1;

        computeRootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        computeRootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        computeRootParameters[7].DescriptorTable.pDescriptorRanges = &uavRanges[2];
        computeRootParameters[7].DescriptorTable.NumDescriptorRanges = 1;

        D3D12_ROOT_SIGNATURE_DESC computeRSDesc{};
        computeRSDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
        computeRSDesc.pParameters = computeRootParameters;
        computeRSDesc.NumParameters = _countof(computeRootParameters);

        Microsoft::WRL::ComPtr<ID3DBlob> computeSignatureBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> computeErrorBlob = nullptr;
        HRESULT hr = D3D12SerializeRootSignature(&computeRSDesc, D3D_ROOT_SIGNATURE_VERSION_1, computeSignatureBlob.GetAddressOf(), computeErrorBlob.GetAddressOf());
        if (FAILED(hr)) {
            Log::OutPutLog(log_->GetLogStream(), reinterpret_cast<char*>(computeErrorBlob->GetBufferPointer()));
            assert(false);
        }
        hr = device_->CreateRootSignature(0, computeSignatureBlob->GetBufferPointer(), computeSignatureBlob->GetBufferSize(), IID_PPV_ARGS(computeRootSignature_.GetAddressOf()));
        assert(SUCCEEDED(hr));
    }
}

void DirectXCommon::CreatePSOs() {
    std::ostream& logStream = log_->GetLogStream();

    // --- シェーダコンパイル ---
    auto vs3d = shaderCompiler_->Compile(L"resources/shaders/Object3D.VS.hlsl", L"vs_6_0", logStream);
    auto ps3d = shaderCompiler_->Compile(L"resources/shaders/Object3D.PS.hlsl", L"ps_6_0", logStream);
    auto vsParticle = shaderCompiler_->Compile(L"resources/shaders/Particle.VS.hlsl", L"vs_6_0", logStream);
    auto psParticle = shaderCompiler_->Compile(L"resources/shaders/Particle.PS.hlsl", L"ps_6_0", logStream);
    auto vsSprite = shaderCompiler_->Compile(L"resources/shaders/Object2D.VS.hlsl", L"vs_6_0", logStream);
    auto psSprite = shaderCompiler_->Compile(L"resources/shaders/Object2D.PS.hlsl", L"ps_6_0", logStream);
    auto vsRegion = shaderCompiler_->Compile(L"resources/shaders/Region.VS.hlsl", L"vs_6_0", logStream);
    auto vsGeo = shaderCompiler_->Compile(L"resources/shaders/ByGeometryShader.VS.hlsl", L"vs_6_0", logStream);
    auto psGeo = shaderCompiler_->Compile(L"resources/shaders/ByGeometryShader.PS.hlsl", L"ps_6_0", logStream);
    auto gsGeo = shaderCompiler_->Compile(L"resources/shaders/ByGeometryShader.GS.hlsl", L"gs_6_0", logStream);
    auto vsLine = shaderCompiler_->Compile(L"resources/shaders/Line.VS.hlsl", L"vs_6_0", logStream);
    auto psLine = shaderCompiler_->Compile(L"resources/shaders/Line.PS.hlsl", L"ps_6_0", logStream);
    auto vsLineInst = shaderCompiler_->Compile(L"resources/shaders/LineInstanced.VS.hlsl", L"vs_6_0", logStream);
    auto psLineInst = shaderCompiler_->Compile(L"resources/shaders/LineInstanced.PS.hlsl", L"ps_6_0", logStream);
    auto vsSkin = shaderCompiler_->Compile(L"resources/shaders/SkinningObject3D.VS.hlsl", L"vs_6_0", logStream);
    auto vsSkybox = shaderCompiler_->Compile(L"resources/shaders/Skybox.VS.hlsl", L"vs_6_0", logStream);
    auto psSkybox = shaderCompiler_->Compile(L"resources/shaders/Skybox.PS.hlsl", L"ps_6_0", logStream);
    auto vsGpuParticle = shaderCompiler_->Compile(L"resources/shaders/ParticleGPU.VS.hlsl", L"vs_6_0", logStream);
    auto psGpuParticle = shaderCompiler_->Compile(L"resources/shaders/ParticleGPU.PS.hlsl", L"ps_6_0", logStream);
    auto vsVoxel = shaderCompiler_->Compile(L"resources/shaders/VoxelParticle.VS.hlsl", L"vs_6_0", logStream);
    auto psVoxel = shaderCompiler_->Compile(L"resources/shaders/VoxelParticle.PS.hlsl", L"ps_6_0", logStream);
    auto vsShadow = shaderCompiler_->Compile(L"resources/shaders/ShadowMap.VS.hlsl", L"vs_6_0", logStream);
    auto vsShadowSkin = shaderCompiler_->Compile(L"resources/shaders/ShadowMapSkinning.VS.hlsl", L"vs_6_0", logStream);
    auto psLightning = shaderCompiler_->Compile(L"resources/shaders/LightningCrawl.PS.hlsl", L"ps_6_0", logStream);

    auto csSkin = shaderCompiler_->Compile(L"resources/shaders/Skinning.CS.hlsl", L"cs_6_0", logStream);
    auto csGpuInit = shaderCompiler_->Compile(L"resources/shaders/InitializeParticle.CS.hlsl", L"cs_6_0", logStream);
    auto csGpuEmit = shaderCompiler_->Compile(L"resources/shaders/EmitParticle.CS.hlsl", L"cs_6_0", logStream);
    auto csGpuUpdate = shaderCompiler_->Compile(L"resources/shaders/UpdateParticle.CS.hlsl", L"cs_6_0", logStream);
    auto csVoxelInit = shaderCompiler_->Compile(L"resources/shaders/InitializeVoxel.CS.hlsl", L"cs_6_0", logStream);
    auto csVoxelEmit = shaderCompiler_->Compile(L"resources/shaders/EmitVoxel.CS.hlsl", L"cs_6_0", logStream);
    auto csVoxelUpdate = shaderCompiler_->Compile(L"resources/shaders/UpdateVoxel.CS.hlsl", L"cs_6_0", logStream);

    // --- 入力レイアウト定義 ---
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "WEIGHT",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "INDEX",    0, DXGI_FORMAT_R32G32B32A32_SINT,  1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // --- PSOManagerの初期化 ---
    psoManager_ = std::make_unique<PSOManager>();
    psoManager_->Initialize(
        device_.Get(),
        rootSignature_.Get(),
        { inputElementDescs, _countof(inputElementDescs) },
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        { vs3d, ps3d },
        { vsParticle, psParticle },
        { vsSprite, psSprite },
        { vsRegion, ps3d },
        { vsGeo, psGeo, gsGeo },
        { vsLine, psLine },
        { vsLineInst, psLineInst },
        { vsSkin, ps3d },
        { vsSkybox, psSkybox },
        { vsGpuParticle, psGpuParticle },
        { vsVoxel, psVoxel },
        { vsShadow, nullptr },     // shadowShaders
        { vsShadowSkin, nullptr }, // shadowSkinningShaders
        { vs3d, psLightning }     // lightningShaders [NEW]
    );

    // --- Compute PSO生成 ---
    auto createComputePSO = [&](const Microsoft::WRL::ComPtr<IDxcBlob>& blob, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pso) {
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
        desc.pRootSignature = computeRootSignature_.Get();
        desc.CS = { blob->GetBufferPointer(), blob->GetBufferSize() };
        HRESULT hr = device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(pso.GetAddressOf()));
        assert(SUCCEEDED(hr));
    };

    createComputePSO(csSkin, skinningComputePSO_);
    createComputePSO(csGpuInit, gpuParticleInitializePSO_);
    createComputePSO(csGpuEmit, gpuParticleEmitPSO_);
    createComputePSO(csGpuUpdate, gpuParticleUpdatePSO_);
    createComputePSO(csVoxelInit, voxelParticleInitializePSO_);
    createComputePSO(csVoxelEmit, voxelParticleEmitPSO_);
    createComputePSO(csVoxelUpdate, voxelParticleUpdatePSO_);

    // --- ビューポート・シザー矩形設定 ---
    viewport_.Width = static_cast<FLOAT>(clientWidth_);
    viewport_.Height = static_cast<FLOAT>(clientHeight_);
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    scissorRect_.left = 0;
    scissorRect_.right = clientWidth_;
    scissorRect_.top = 0;
    scissorRect_.bottom = clientHeight_;
}


/*開発用のUIを出そう*/

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
    descriptorHeapDesc.Type = heapType;
    descriptorHeapDesc.NumDescriptors = numDescriptors;
    descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(descriptorHeap.GetAddressOf()));
    assert(SUCCEEDED(hr));
    return descriptorHeap;

}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handleCPU.ptr += (descriptorSize * index);
    return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
    D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handleGPU.ptr += (descriptorSize * index);
    return handleGPU;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetRTVCPUDescriptorHandle(uint32_t index) {

    return GetCPUDescriptorHandle(rtvDescriptorHeap_, descriptorSizeRTV_, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetRTVGPUDescriptorHandle(uint32_t index) {

    return GetGPUDescriptorHandle(rtvDescriptorHeap_, descriptorSizeRTV_, index);
}


D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetDSVCPUDescriptorHandle(uint32_t index) {

    return GetCPUDescriptorHandle(dsvDescriptorHeap_, descriptorSizeDSV_, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetDSVGPUDescriptorHandle(uint32_t index) {

    return GetGPUDescriptorHandle(dsvDescriptorHeap_, descriptorSizeDSV_, index);
}

uint32_t DirectXCommon::AllocateRTVIndex() {
    std::lock_guard<std::mutex> lock(pendingMutex_);
    if (!freeRtvIndices_.empty()) {
        uint32_t index = freeRtvIndices_.back();
        freeRtvIndices_.pop_back();
        return index;
    }
    assert(nextRtvIndex_ < 128);
    return nextRtvIndex_++;
}

uint32_t DirectXCommon::AllocateDSVIndex() {
    std::lock_guard<std::mutex> lock(pendingMutex_);
    if (!freeDsvIndices_.empty()) {
        uint32_t index = freeDsvIndices_.back();
        freeDsvIndices_.pop_back();
        return index;
    }
    assert(nextDsvIndex_ < 16);
    return nextDsvIndex_++;
}

void DirectXCommon::FreeRTVIndex(uint32_t index) {
    if (index == 0xFFFFFFFF) return;
    std::lock_guard<std::mutex> lock(pendingMutex_);
    pendingFreeRtvs_.push_back({ globalFenceValue_, index });
}

void DirectXCommon::FreeDSVIndex(uint32_t index) {
    if (index == 0xFFFFFFFF) return;
    std::lock_guard<std::mutex> lock(pendingMutex_);
    pendingFreeDsvs_.push_back({ globalFenceValue_, index });
}

/*三角形の色を変えよう*/

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes) {

    ///BufferResourceを生成する

    //頂点リソース用のヒープを生成
    D3D12_HEAP_PROPERTIES uploadHeapProperties{};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; //UploadHeapを使う
    //頂点リソースの設定
    D3D12_RESOURCE_DESC vertexResourceDesc{};
    //バッファリソース、テクスチャの場合はまた別の設定をする
    vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexResourceDesc.Width = sizeInBytes; //リソースのサイズ。今回はVector4を3頂点分
    //バッファの場合はこれらは1にする決まり
    vertexResourceDesc.Height = 1;
    vertexResourceDesc.DepthOrArraySize = 1;
    vertexResourceDesc.MipLevels = 1;
    vertexResourceDesc.SampleDesc.Count = 1;
    //バッファの場合はこれにする決まり
    vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    //実際に頂点リソースを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource = nullptr;
    HRESULT hr = device_->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(bufferResource.GetAddressOf()));
    assert(SUCCEEDED(hr));

    return bufferResource;

}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateUAVBufferResource(size_t sizeInBytes) {
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = sizeInBytes;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource = nullptr;
    HRESULT hr = device_->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(bufferResource.GetAddressOf()));
    assert(SUCCEEDED(hr));

    return bufferResource;
}

Microsoft::WRL::ComPtr<ID3D12Resource>  DirectXCommon::UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages) {
	// --- スレッドセーフ化 ---
	std::lock_guard<std::mutex> lock(uploadMutex_);

    ///IntermediateResource(中間リソース)
    std::vector<D3D12_SUBRESOURCE_DATA> subResources;
    DirectX::PrepareUpload(device_.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subResources);
    uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subResources.size()));
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(intermediateSize);

    ///専用コマンドリストのリセットと記録
	uploadCommandAllocator_->Reset();
	uploadCommandList_->Reset(uploadCommandAllocator_.Get(), nullptr);

    UpdateSubresources(uploadCommandList_.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subResources.size()), subResources.data());

    //Textureへの転送後は利用できるよう、ResourceStateを変更
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    uploadCommandList_->ResourceBarrier(1, &barrier);

	// 実行
	uploadCommandList_->Close();
	ID3D12CommandList* ppCommandLists[] = { uploadCommandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, ppCommandLists);

	// FenceによるGPU完了待ちを非同期化
	uploadFenceValue_++;
	commandQueue_->Signal(uploadFence_.Get(), uploadFenceValue_);
	
	// 中間リソースを遅延解放に登録
	ReleaseAfterFence(intermediateResource);

    return intermediateResource;
}

/*テクスチャを貼ろう*/

///DirectX12のTextureResourceを作る

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateTextureResource(const DirectX::TexMetadata& metadata) {
    //1. metadataを基にResourceの設定
    //2. 利用するHeapの設定
    //3. Resourceを生成する

    /*テクスチャを正しく配置しよう*/

    ///正式な手順

    //before
    //1. TextureデータそのものをCPUに読み込む
    //2. DirectX12のTextureResourceを作る)(MainMemory)
    //3. TextureResourceに1で読んだデータを転送する(WriteToSubResource)

    //after
    //1.Textureデータその物をCPUで読み込む

    //2. DirectX12TextureResourceを作る(VRAM)
    //3. CPUに書き込む用にUploadHeapのResourceを作る(IntermediateResource)
    //4. 3に対してCPUでデータを書き込む
    //5. CommandListに3を2に転送するコマンドを積む
    //6. CommandQueueを使って実行する
    //7. 6の事項完了を待つ

    /*テクスチャを貼ろう*/

    //metadataを基にResourceの設定
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = UINT(metadata.width); //Textureの幅
    resourceDesc.Height = UINT(metadata.height); //Textureの高さ
    resourceDesc.MipLevels = UINT(metadata.mipLevels); //mipmapの数
    resourceDesc.DepthOrArraySize = UINT(metadata.arraySize); // 奥行きor 配列Textureの配列数
    resourceDesc.Format = metadata.format; //TextureのFormat
    resourceDesc.SampleDesc.Count = 1; //サンプリングカウント。1固定。
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); //Textureの次元数。普段使っているのは2次元。

    //利用するHeapの設定。非常に特殊な運用。02_04exで一般的なケース版がある
    D3D12_HEAP_PROPERTIES heapProperties{};
    //heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM; //細かい設定を行う
    // heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
    //heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; //プロセッサの近くに配置

    /*テクスチャを正しく配置しよう*/

    //TextureResourceを作る(VRAM)

    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    /*テクスチャを貼ろう*/

    //Resourceの生成
    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    HRESULT hr = device_->CreateCommittedResource(
        &heapProperties, //Heapの設定
        D3D12_HEAP_FLAG_NONE, //Heapの特殊な設定。特になし。
        &resourceDesc, //Resourceの設定
        //D3D12_RESOURCE_STATE_GENERIC_READ, //初回のResourceState。Textureは基本読むだけ。

        /*テクスチャを正しく配置しよう*/

        D3D12_RESOURCE_STATE_COPY_DEST, //データ転送される設定

        /*テクスチャを貼ろう*/

        nullptr, //Clear最適値。使わないのでnullptr
        IID_PPV_ARGS(resource.GetAddressOf()) //作成するResourceポインタへのポインタ
    );
    assert(SUCCEEDED(hr));
    return resource;
}

/*テクスチャを貼ろう*/

///Textureデータを読む

DirectX::ScratchImage DirectXCommon::LoadTexture(const std::string& filePath) {
    using namespace DirectX;

    ScratchImage image{};
    std::wstring filePathW = ConvertString(filePath);

    // Win32 API を使用してファイルの存在確認を行う（std::filesystem の代用）
    if (GetFileAttributesW(filePathW.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::wstring msg = L"[LoadTexture] File not found: " + filePathW + L"\n";
        OutputDebugStringW(msg.c_str());
        assert(false && "Texture file not found");
    }

    // --- sRGB 判定ロジック ---
    bool isSRGB = true;
    std::string filePathLower = filePath;
    std::transform(filePathLower.begin(), filePathLower.end(), filePathLower.begin(), ::tolower);
    if (filePathLower.find("_n") != std::string::npos ||
        filePathLower.find("normal") != std::string::npos ||
        filePathLower.find("_ao") != std::string::npos ||
        filePathLower.find("_m") != std::string::npos ||
        filePathLower.find("metallic") != std::string::npos ||
        filePathLower.find("_r") != std::string::npos ||
        filePathLower.find("roughness") != std::string::npos) {
        isSRGB = false;
    }

    HRESULT hr;
    TextureUtility::TextureFileType fileType = TextureUtility::GetTextureFileType(filePathW);

    if (fileType == TextureUtility::TextureFileType::DDS) {
        hr = LoadFromDDSFile(filePathW.c_str(), DDS_FLAGS_NONE, nullptr, image);
        
        // 8bit 系のフォーマットかつカラーマップ（normal等でない）であれば sRGB 形式へ変更
        if (SUCCEEDED(hr) && isSRGB) {
            const auto& metadata = image.GetMetadata();
            if (DirectX::FormatDataType(metadata.format) != DirectX::FORMAT_TYPE_FLOAT) {
                image.OverrideFormat(DirectX::MakeSRGB(metadata.format));
            }
        }
    }
    else {
        WIC_FLAGS wicFlags = isSRGB ? WIC_FLAGS_FORCE_SRGB : WIC_FLAGS_NONE;
        hr = LoadFromWICFile(filePathW.c_str(), wicFlags, nullptr, image);
    }

    if (FAILED(hr)) {
        _com_error err(hr);
        std::wstring msg = L"[LoadTexture] Load failed (" + std::to_wstring(hr) +
            L"): " + filePathW + L" - ";
#ifdef UNICODE
        msg += err.ErrorMessage();
#else
        msg += ConvertString(err.ErrorMessage());
#endif
        msg += L"\n";
        OutputDebugStringW(msg.c_str());
        assert(false && "LoadTexture failed");
    }

    ScratchImage mipImages{};
    if (IsCompressed(image.GetMetadata().format)) {
        mipImages = std::move(image);
    }
    else {
        TEX_FILTER_FLAGS filter = isSRGB ? TEX_FILTER_SRGB : TEX_FILTER_DEFAULT;
        hr = GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
            filter, 0, mipImages);
        if (FAILED(hr)) {
            _com_error err(hr);
            std::wstring msg = L"[LoadTexture] GenerateMipMaps failed (" + std::to_wstring(static_cast<unsigned long>(hr)) + L")\n";
            OutputDebugStringW(msg.c_str());
            assert(false && "GenerateMipMaps failed");
        }
    }

    return mipImages;
}


DirectX::TexMetadata DirectXCommon::GetTextureMetadata(const std::string& filePath) {
    using namespace DirectX;
    std::wstring filePathW = ConvertString(filePath);
    TexMetadata metadata{};
    if (StringUtility::EndsWith(filePathW, L".dds")) {
        GetMetadataFromDDSFile(filePathW.c_str(), DDS_FLAGS_NONE, metadata);
    }
    else {
        GetMetadataFromWICFile(filePathW.c_str(), WIC_FLAGS_NONE, metadata);
    }
    return metadata;
}


Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateDepthStencilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height) {

    ///Resource/Heapの設定を行う

    //生成するResourceの設定
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = width; //Textureの幅
    resourceDesc.Height = height; //Textureの高さ
    resourceDesc.MipLevels = 1; //mipmapの数
    resourceDesc.DepthOrArraySize = 1; //奥行き or 配列Textureの配列数
    resourceDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // 深度バッファとして使いつつ、SRVで読み込むためにTYPELESSにする
    resourceDesc.SampleDesc.Count = 1; //サンプリングカウント。1固定
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; //2次元
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

    //利用するHeapの設定
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; //VRAM上に作る

    ///深度地のクリア最適化設定

    //深度地のクリア設定
    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.DepthStencil.Depth = 1.0f; // 1.0f(最大値)でクリア
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // クリアには実際の深度フォーマットを指定

    ///Resourceの生成

    //Resourceの生成
    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties, //Heapの設定
        D3D12_HEAP_FLAG_NONE, //Heapの特殊な設定。特になし。
        &resourceDesc, //Resourceの設定
        D3D12_RESOURCE_STATE_DEPTH_WRITE, //深度値を書き込む状態にしておく
        &depthClearValue, //Clear最適値
        IID_PPV_ARGS(resource.GetAddressOf()) //作成するResourceポインタへのポインタ
    );
    assert(SUCCEEDED(hr));

    return resource;
}

UINT DirectXCommon::GetBackBufferIndex(const Microsoft::WRL::ComPtr<IDXGISwapChain4>& swapChain) {
    assert(swapChain != nullptr);
    return swapChain->GetCurrentBackBufferIndex();
}

void DirectXCommon::PreWarmJITCompile() {
    std::lock_guard<std::mutex> lock(uploadMutex_);
    uploadCommandAllocator_->Reset();
    uploadCommandList_->Reset(uploadCommandAllocator_.Get(), nullptr);

    // --- Compute PSO ---
    uploadCommandList_->SetComputeRootSignature(computeRootSignature_.Get());
    
    // SetPipelineState とダミー Dispatch(0,0,0) を発行し、
    // NVIDIAやIntelのドライバに対し、最初のDraw()より前に強制的に
    // ハードウェア専用のISA（機械語）へJITコンパイルさせる
    ID3D12PipelineState* csPSOs[] = {
        skinningComputePSO_.Get(),
        gpuParticleInitializePSO_.Get(),
        gpuParticleEmitPSO_.Get(),
        gpuParticleUpdatePSO_.Get(),
        voxelParticleInitializePSO_.Get(),
        voxelParticleEmitPSO_.Get(),
        voxelParticleUpdatePSO_.Get()
    };
    for (auto pso : csPSOs) {
        if (pso) {
            uploadCommandList_->SetPipelineState(pso);
        }
    }

    // --- Graphics PSO (重いもの) ---
    uploadCommandList_->SetGraphicsRootSignature(rootSignature_.Get());
    uploadCommandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    if (psoManager_) {
        // 例: 高負荷な特殊パイプライン(電撃エフェクト等)
        auto lightningPSO = psoManager_->GetLightningCrawl(BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None);
        if (lightningPSO) {
            uploadCommandList_->SetPipelineState(lightningPSO);
        }
    }

    uploadCommandList_->Close();
    ID3D12CommandList* ppCommandLists[] = { uploadCommandList_.Get() };
    commandQueue_->ExecuteCommandLists(1, ppCommandLists);

    uploadFenceValue_++;
    commandQueue_->Signal(uploadFence_.Get(), uploadFenceValue_);

    // GPU上のダミー実行とJITコンパイルが完全に終わるのを待機してから進行する
    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (uploadFence_->GetCompletedValue() < uploadFenceValue_) {
        uploadFence_->SetEventOnCompletion(uploadFenceValue_, event);
        WaitForSingleObject(event, INFINITE);
    }
    CloseHandle(event);
}

void DirectXCommon::ExecuteUploadCommands(std::function<void(ID3D12GraphicsCommandList*)> commands) {
    std::lock_guard<std::mutex> lock(uploadMutex_);
    uploadCommandAllocator_->Reset();
    uploadCommandList_->Reset(uploadCommandAllocator_.Get(), nullptr);

    commands(uploadCommandList_.Get());

    uploadCommandList_->Close();
    ID3D12CommandList* ppCommandLists[] = { uploadCommandList_.Get() };
    commandQueue_->ExecuteCommandLists(1, ppCommandLists);

    uploadFenceValue_++;
    commandQueue_->Signal(uploadFence_.Get(), uploadFenceValue_);

    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (uploadFence_->GetCompletedValue() < uploadFenceValue_) {
        uploadFence_->SetEventOnCompletion(uploadFenceValue_, event);
        WaitForSingleObject(event, INFINITE);
    }
    CloseHandle(event);
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device>& device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
    descriptorHeapDesc.Type = heapType;
    descriptorHeapDesc.NumDescriptors = numDescriptors;
    descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(descriptorHeap.GetAddressOf()));
    assert(SUCCEEDED(hr));
    return descriptorHeap;

}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateRenderTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4* clearColor) {
	// 1. metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width; //Textureの幅
	resourceDesc.Height = height; //Textureの高さ
	resourceDesc.MipLevels = 1; //mipmapの数
	resourceDesc.DepthOrArraySize = 1; // 奥行きor 配列Textureの配列数
	resourceDesc.Format = format; //TextureのFormat
	resourceDesc.SampleDesc.Count = 1; //サンプリングカウント。1固定。
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; //Textureの次元数。
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // RenderTargetとして利用可能にする

	// 2. 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 当然VRAM上に作る

	// 3. ClearValueの設定
	D3D12_CLEAR_VALUE clearValue{};
	D3D12_CLEAR_VALUE* pClearValue = nullptr;
	if (clearColor) {
		clearValue.Format = format;
		clearValue.Color[0] = clearColor->x;
		clearValue.Color[1] = clearColor->y;
		clearValue.Color[2] = clearColor->z;
		clearValue.Color[3] = clearColor->w;
		pClearValue = &clearValue;
	}

	// 4. Resourceを生成する
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties, //Heapの設定
		D3D12_HEAP_FLAG_NONE, //Heapの特殊な設定。
		&resourceDesc, //Resourceの設定
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 最初はSRVとして扱える状態で生成する
		pClearValue, // Clear最適値。指定がなければnullptr
		IID_PPV_ARGS(resource.GetAddressOf()) //作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));
	return resource;
}

void DirectXCommon::ReleaseSwapChainResources() {
    for (auto& res : swapChainResources_) {
        res.Reset();
    }
    depthStencilResource_.Reset();
}

void DirectXCommon::ResizeSwapChain(int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) return;

    // 1. GPUの完了を待つ (Flush)
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        uint64_t fv = IncrementGlobalFence();
        commandQueue_->Signal(fence_.Get(), fv);
        if (fence_->GetCompletedValue() < fv) {
            fence_->SetEventOnCompletion(fv, fenceEvent_);
            WaitForSingleObject(fenceEvent_, INFINITE);
        }
    }

    // 2. 既存のリソースを解放 (ResizeBuffersの前に必須)
    ReleaseSwapChainResources();

    // 3. バッファサイズの変更
    DXGI_SWAP_CHAIN_DESC1 desc{};
    swapChain_->GetDesc1(&desc);
    HRESULT hr = swapChain_->ResizeBuffers(desc.BufferCount, width, height, desc.Format, desc.Flags);
    assert(SUCCEEDED(hr));

    clientWidth_ = width;
    clientHeight_ = height;

    // 4. バックバッファの再取得とRTVの再作成
    for (uint32_t i = 0; i < desc.BufferCount; ++i) {
        hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(swapChainResources_[i].GetAddressOf()));
        assert(SUCCEEDED(hr));
        
        // メイン RTV (SRGB)
        device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc_, rtvHandles_[i]);

        // ImGui用 RTV (UNORM) も再作成
        D3D12_RENDER_TARGET_VIEW_DESC imGuiRtvDesc = rtvDesc_;
        imGuiRtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtvHandles_[i + 2].ptr = rtvHandles_[1].ptr + ((i + 1) * descriptorSizeRTV_);
        device_->CreateRenderTargetView(swapChainResources_[i].Get(), &imGuiRtvDesc, rtvHandles_[i + 2]);
    }

    // 5. 深度バッファの再生成とDSVの再作成
    depthStencilResource_ = CreateDepthStencilTextureResource(device_.Get(), width, height);
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

    // 6. ビューポートとシザーレクトの更新
    viewport_.Width = static_cast<float>(width);
    viewport_.Height = static_cast<float>(height);
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    scissorRect_.left = 0;
    scissorRect_.top = 0;
    scissorRect_.right = width;
    scissorRect_.bottom = height;
}
 
void DirectXCommon::ReleaseAfterFence(Microsoft::WRL::ComPtr<ID3D12Resource> resource) {
	if (!resource) return;
	std::lock_guard<std::mutex> lock(pendingMutex_);
	pendingResources_.push_back({ globalFenceValue_ + 1, resource });
}
 
void DirectXCommon::ClearPendingResources() {
	uint64_t completed = fence_->GetCompletedValue();
	std::lock_guard<std::mutex> lock(pendingMutex_);

	// リソースの回収
	auto it = std::remove_if(pendingResources_.begin(), pendingResources_.end(), [completed](const PendingResource& res) {
		return res.fenceValue <= completed;
	});
	pendingResources_.erase(it, pendingResources_.end());

	// RTV デスクリプタの回収
	auto rtvIt = std::remove_if(pendingFreeRtvs_.begin(), pendingFreeRtvs_.end(), [completed](const PendingDescriptor& d) {
		return d.fenceValue <= completed;
	});
	for (auto d = rtvIt; d != pendingFreeRtvs_.end(); ++d) {
		freeRtvIndices_.push_back(d->index);
	}
	pendingFreeRtvs_.erase(rtvIt, pendingFreeRtvs_.end());

	// DSV デスクリプタの回収
	auto dsvIt = std::remove_if(pendingFreeDsvs_.begin(), pendingFreeDsvs_.end(), [completed](const PendingDescriptor& d) {
		return d.fenceValue <= completed;
	});
	for (auto d = dsvIt; d != pendingFreeDsvs_.end(); ++d) {
		freeDsvIndices_.push_back(d->index);
	}
	pendingFreeDsvs_.erase(dsvIt, pendingFreeDsvs_.end());
}
