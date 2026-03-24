#include "DirectXCommon.h"

#include <string>
#include <array>
#include <cassert>
#include <format>
#include <filesystem>
#include <comdef.h>

#include "Engine/Core/Utility/Log.h"
#include "Engine/Core/Utility/StringUtility.h"
#include "Renderer/VertexData.h"
#include "DirectXTex/d3dx12.h"
#include <thread>

void DirectXCommon::Finalize() {


    // GPU同期
    if (commandQueue_ && fence_) {
        commandQueue_->Signal(fence_.Get(), ++fenceValue_);
        if (fence_->GetCompletedValue() < fenceValue_) {
            fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
            WaitForSingleObject(fenceEvent_, INFINITE);
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
    voxelParticleUpdatePSO_.Reset();
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
    commandList_.Reset();
    commandAllocator_.Reset();
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

    InitializeFixFPS();

    /*エラー放置ダメ、絶対*/

    ///DebugLayer(デバッグレイヤー)

#if defined(_DEBUG) || defined(DEVELOPMENT)

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController_.GetAddressOf())))) {
        //デバッグレイヤーを有効化する
        debugController_->EnableDebugLayer();
        //さらにGPU側でもチェックを行うようにする
        debugController_->SetEnableGPUBasedValidation(TRUE);
    }
#endif

    /*DirectX12を初期化しよう*/

    ///DXGIFactoryの生成

    //HRESULTはWindows系のエラーコードであり、
    //関数が成功したかどうかをSUCCEEDEDマクロで判定できる
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory_.GetAddressOf()));
    //初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、どうにもできない場合が多いのでassertにしておく
    assert(SUCCEEDED(hr));

    ///使用するアダプタ(GPU)を決定する

    //使用するアダプタ用の変数。最初にnullptrを入れておく
    Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
    //良い順にアダプタを頼む
    for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(useAdapter.GetAddressOf())) != DXGI_ERROR_NOT_FOUND; i++) {
        //アダプター情報を取得する
        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr)); //取得できないのは一大事
        //ソフトウェアアダプタでなければ採用!
        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            //採用したアダプタの情報をログに出力。wstringの方なので注意
            Log::OutPutLog(log_->GetLogStream(), ConvertString(std::format(L"use Adapter:{}\n", adapterDesc.Description)));
            break;
        }
        useAdapter = nullptr; //ソフトウェアアダプタの場合は見なかったことにする
    }
    //適切なアダプタが見つからなかったので起動できない
    assert(useAdapter != nullptr);

    ///D3D12Deviceの生成

    //機能レベルとログ出力用の文字列
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
    };
    const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
    //高い順に生成できるか試してく
    for (size_t i = 0; i < _countof(featureLevels); ++i) {
        //採用したアダプターでデバイスを生成
        hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(device_.GetAddressOf()));

        //指定した機能レベルでデバイスが生成できたかを確認
        if (SUCCEEDED(hr)) {
            Log::OutPutLog(log_->GetLogStream(), std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
            auto msg = std::format("Resource[{}] created at {} in {}:{}\n", "ID3D12Device", static_cast<const void*>(device_.Get()), __FILE__, __LINE__);
            OutputDebugStringA(msg.c_str());
            break;
        }
    }
    //デバイス生成がうまくいかなかったので起動できない
    assert(device_ != nullptr);
    Log::OutPutLog(log_->GetLogStream(), "Complete create D3D12Device!!!\n"); //初期化完了のログを出す

    // 生成が完了したのでuseAdapterを解放
    if (useAdapter) { useAdapter.Reset(); }


    /*エラー放置ダメ、絶対*/

    ///エラー・警告、即ち停止

#if defined(_DEBUG) || defined(DEVELOPMENT)
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        //ヤバイエラー時に止まる
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        //エラー時に止まる
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        //警告時に止まる
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

        ///エラーと警告の抑制(Windowsの不具合によるエラー表記などは解消不能。その場合に停止させないよう設定を行う)

        //抑制するメッセージのID
        D3D12_MESSAGE_ID denyIds[] = {
            //Windows11でのDXGIデバッグプレイヤーとDX12デバッグプレイヤーの相互作用バグによるエラーメッセージ
            //https://stackoverflow.com/questions/69805245/directXx-12-application-is-crashing-in-windows-11
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
            // クリアカラーが動的な場合に発生するパフォーマンス警告を抑制
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
        };
        //抑制するレベル
        D3D12_MESSAGE_SEVERITY severties[] = { D3D12_MESSAGE_SEVERITY_INFO };
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severties);
        filter.DenyList.pSeverityList = severties;
        //指定したメッセージの表示を抑制する
        infoQueue->PushStorageFilter(&filter);

        ///エラー・警告、即ち停止

        //解放
        infoQueue.Reset();
    }
#endif

    /*画面の色を変えよう*/

    ///CommandQueueを生成する

    //コマンドキューを生成する
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue_.GetAddressOf()));
    //コマンドキューの生成がうまくいかないので起動できない
    assert(SUCCEEDED(hr));

    ///CommandListを生成する

    //コマンドアロケータを生成する
    hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocator_.GetAddressOf()));
    //コマンドアロケータの生成がうまくいかなかったので起動できない
    assert(SUCCEEDED(hr));

    //コマンドリストを生成する
    hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(commandList_.GetAddressOf()));
    //コマンドリストの生成がうまくいかなかったので起動できない
    assert(SUCCEEDED(hr));

    ///SwapChainを生成する

    //スワップチェーンを生成する
    swapChainDesc_.Width = clientWidth_; //画面の幅。ウィンドウのクライアント領域を同じものにしておく
    swapChainDesc_.Height = clientHeight_; //画面の高さ。ウィンドウのクライアント領域を同じものにしておく
    swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //色の形式
    swapChainDesc_.SampleDesc.Count = 1; //マルチサンプルしない
    swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //描画のターゲットとして利用する
    swapChainDesc_.BufferCount = 2; //ダブルバッファ
    swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //モニタにうつしたら、中身を破棄
    //コマンドキュー、ウィンドウハンドル、設定を渡して生成する
    hr = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_.Get(), hwnd_, &swapChainDesc_, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
    assert(SUCCEEDED(hr));

    // 作成したのでdxgiFactoryを解放
    if (dxgiFactory_) { dxgiFactory_.Reset(); }


    /*テクスチャを切り替えよう*/

    //DescriptorSize

    descriptorSizeRTV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptorSizeDSV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    /*開発用のUIを出そう*/

    //作成関数を使う

    //RTV用のヒープでデスクリプタの数は32。RTVはShader内で触るものではないので、ShaderVisibleはfalse
    rtvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 32, false);
    nextRtvIndex_ = 2; // 0, 1 は SwapChain 用

    //SRV用のヒープをDescriptorPoolで作成
    srvPool_ = std::make_unique<DescriptorPool>();
    srvPool_->Initialize(device_.Get());

    /*画面の色を変えよう*/

    ///SwapChainからResourceを引っ張ってくる

    //SwapChainからResourceを引っ張ってくる
    hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(swapChainResources_[0].GetAddressOf()));
    //うまく取得できなければ起動できない
    assert(SUCCEEDED(hr));
    hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(swapChainResources_[1].GetAddressOf()));
    assert(SUCCEEDED(hr));

    ///RTVを作る

    //RTVの設定
    rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //出力結果をSRGBに変換して書き込む
    rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; //2Dテクスチャとして書き込む
    //ディスクリプタの先頭を取得する
    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    //RTVを2つ作るのでディスクリプタを2つ用意
    //まず1つ目を作る。1つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある
    rtvHandles_[0] = rtvStartHandle;
    device_->CreateRenderTargetView(swapChainResources_[0].Get(), &rtvDesc_, rtvHandles_[0]);
    //2つ目のディスクリプタハンドルを得る(自力で)
    rtvHandles_[1].ptr = rtvHandles_[0].ptr + device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    //2つ目を作る
    device_->CreateRenderTargetView(swapChainResources_[1].Get(), &rtvDesc_, rtvHandles_[1]);

    /*前後関係を正しくしよう*/

    //DepthStencilTextureをウィンドウのサイズで作成
    depthStencilResource_ = CreateDepthStencilTextureResource(device_.Get(), clientWidth_, clientHeight_);

    ///DepthStencilView(DSV)

    //DSV用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
    dsvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

    //DSVの設定
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; //Format。基本的にはResourceに合わせる
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; //2dTexture
    //DSVHeapの先頭にDSVをつける
    device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

    /*完璧な画面クリアを目指して*/

    ///FenceとEventを生成する

    //初期値0でFenceを作る
    hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf()));
    assert(SUCCEEDED(hr));

    //FenceのSignalを待つためのイベントを作成する
    fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent_ != nullptr);

    /*三角形を表示しよう*/

    ///DXCの初期化

    //dxcCompilerを初期化
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
    Microsoft::WRL::ComPtr <IDxcCompiler3> dxcCompiler = nullptr;
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils.GetAddressOf()));
    assert(SUCCEEDED(hr));
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(dxcCompiler.GetAddressOf()));
    assert(SUCCEEDED(hr));

    //現時点でincludeはしないが、includeに対応するための設定を行っておく
    Microsoft::WRL::ComPtr <IDxcIncludeHandler> includeHandler = nullptr;
    hr = dxcUtils->CreateDefaultIncludeHandler(includeHandler.GetAddressOf());
    assert(SUCCEEDED(hr));

    ///RootSignatureを生成する

    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    /*テクスチャを貼ろう*/

    ///RootSignatureを書き換える

    ///DescriptorRange

    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0; //0からは始まる
    descriptorRange[0].NumDescriptors = 1; //数は1つ
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; //SRVを使う
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; //offsetを自動計算

    /*たくさんの板ポリを出そう*/

    /// RootSignatureの変更

    D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
    descriptorRangeForInstancing[0].BaseShaderRegister = 0; //0からは始まる
    descriptorRangeForInstancing[0].NumDescriptors = 1; //数は1つ
    descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; //SRVを使う
    descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; //offsetを自動計算

    /*テクスチャを貼ろう*/

    ///DescriptorTable

    D3D12_ROOT_PARAMETER rootParameters[13] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; //CBVを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; //PixelShaderで使う
    rootParameters[0].Descriptor.ShaderRegister = 0; //レジスタ番号0を使う

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; //CBVを使う
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; //VertexShaderで使う
    rootParameters[1].Descriptor.ShaderRegister = 0; //レジスタ番号0を使う

    /*テクスチャを貼ろう*/

    ///DescriptorTable

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; //descriptorTableを使う
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; //PixelShaderで使う
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange; //Tableの中身の配列を指定
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); //Tableで利用する数

    /*LambertianReflectance*/

    ///平行光源をShaderで使う

    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; //CBVを使う
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // ← ここを変更
    rootParameters[3].Descriptor.ShaderRegister = 1; //レジスタ番号1を使う


    rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[4].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
    rootParameters[4].DescriptorTable.NumDescriptorRanges = 1;

    /*PhongReflectionModel*/

    /// カメラの位置を送る

    rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PS で使う
    rootParameters[5].Descriptor.ShaderRegister = 2;

    /*PointLight*/

    /// PointLightを定義する

    // PointLight (PS, b3)
    rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PS で使う
    rootParameters[6].Descriptor.ShaderRegister = 3; // b3

    /*SpotLight*/

    /// SpotLightを定義する

    // SpotLight (PS, b3)
    rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PS で使う
    rootParameters[7].Descriptor.ShaderRegister = 4; // b4

    // GSパイプライン用のTransformationMatrix (VS/GS, b6)
    rootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // VSとGSで共有
    rootParameters[8].Descriptor.ShaderRegister = 6; // レジスタ番号6を使用

    // ParticleMaterial (PS, b5)
    rootParameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[9].Descriptor.ShaderRegister = 5;

    // AreaLight (PS, b7)
    rootParameters[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PS で使う
    rootParameters[10].Descriptor.ShaderRegister = 7; // b7

    // Line Instancing (VS, t1)
    D3D12_DESCRIPTOR_RANGE descriptorRangeForLineInstancing[1] = {};
    descriptorRangeForLineInstancing[0].BaseShaderRegister = 1; // t1
    descriptorRangeForLineInstancing[0].NumDescriptors = 1;
    descriptorRangeForLineInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRangeForLineInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[11].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[11].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[11].DescriptorTable.pDescriptorRanges = descriptorRangeForLineInstancing;
    rootParameters[11].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForLineInstancing);

    // EnvironmentMap (PS, t1)
    D3D12_DESCRIPTOR_RANGE descriptorRangeForEnvMap[1] = {};
    descriptorRangeForEnvMap[0].BaseShaderRegister = 1; // t1
    descriptorRangeForEnvMap[0].NumDescriptors = 1;
    descriptorRangeForEnvMap[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRangeForEnvMap[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[12].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[12].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[12].DescriptorTable.pDescriptorRanges = descriptorRangeForEnvMap;
    rootParameters[12].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForEnvMap);


    /*テクスチャを貼ろう*/

    ///Samplerの設定

    D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
    // WRAP Sampler (s0)
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; //バイリニアフィルタ
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //0~1の範囲外をリピート
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; //比較しない
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; //ありったけのMIpmapを使う
    staticSamplers[0].ShaderRegister = 0; //レジスタ番号0を使う
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; //PixelShaderで使う
    // CLAMP Sampler (s1)
    staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[1].ShaderRegister = 1; // レジスタ番号1を使う
    staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    /*三角形の色を変えよう*/

    ///RootParameter

    descriptionRootSignature.pParameters = rootParameters; //ルートパラメータ配列へのポインタ
    descriptionRootSignature.NumParameters = _countof(rootParameters); //配列の長さ

    ///register(解説)

    //rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; //b0のbと一致する
    //rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    //rootParameters[0].Descriptor.ShaderRegister = 0; //b0のbと一致する。もしb11と紐づけたいなら11となる

    /*三角形を表示しよう*/

    ///RootSignatureを生成する

    //シリアライズしてバイナリにする
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob>  errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, signatureBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        Log::OutPutLog(log_->GetLogStream(), reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }
    //バイナリを元に生成
    hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(rootSignature_.GetAddressOf()));
    assert(SUCCEEDED(hr));

    // 生成が完了したのでsignatureBlob、errorBlobを解放
    if (signatureBlob) { signatureBlob.Reset(); }
    if (errorBlob) { errorBlob.Reset(); }

    // --- Compute Shader用のRootSignatureとPSOを生成 ---
    {
        D3D12_ROOT_SIGNATURE_DESC computeRootSignatureDesc{};
        computeRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        // Skinning.CS.hlslのresource bindingに合わせる
        // t0: StructuredBuffer<Well> gMatrixPalette
        // t1: StructuredBuffer<Vertex> gInputVertices
        // t2: StructuredBuffer<VertexInfluence> gInfluences
        // u0: RWStructuredBuffer<Vertex> gOutputVertices
        // b0: ConstantBuffer<SkinningInformation> gSkinningInformation
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
        computeRootParameters[4].Descriptor.RegisterSpace = 0;

        computeRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        computeRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        computeRootParameters[5].Descriptor.ShaderRegister = 1; // b1
        computeRootParameters[5].Descriptor.RegisterSpace = 0;

        computeRootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        computeRootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        computeRootParameters[6].DescriptorTable.pDescriptorRanges = &uavRanges[1];
        computeRootParameters[6].DescriptorTable.NumDescriptorRanges = 1;

        computeRootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        computeRootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        computeRootParameters[7].DescriptorTable.pDescriptorRanges = &uavRanges[2];
        computeRootParameters[7].DescriptorTable.NumDescriptorRanges = 1;

        computeRootSignatureDesc.pParameters = computeRootParameters;
        computeRootSignatureDesc.NumParameters = _countof(computeRootParameters);

        Microsoft::WRL::ComPtr<ID3DBlob> computeSignatureBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> computeErrorBlob = nullptr;
        hr = D3D12SerializeRootSignature(&computeRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &computeSignatureBlob, &computeErrorBlob);
        if (FAILED(hr)) {
            Log::OutPutLog(log_->GetLogStream(), reinterpret_cast<char*>(computeErrorBlob->GetBufferPointer()));
            assert(false);
        }
        hr = device_->CreateRootSignature(0, computeSignatureBlob->GetBufferPointer(), computeSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&computeRootSignature_));
        assert(SUCCEEDED(hr));
    }

    /*テクスチャを貼ろう*/

    ///InputLayoutの拡張

    // D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};

    /*Skinning*/

    /// InputLayoutの拡張

    std::array<D3D12_INPUT_ELEMENT_DESC, 5> inputElementDescs{};

    /*テクスチャを貼ろう*/

    ///InputLayoutの拡張
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    /*LambertianReflectance*/

    ///法線の定義を追加する

    inputElementDescs[2].SemanticName = "NORMAL";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    /*Skinning*/

    /// InputLayoutの拡張
    inputElementDescs[3].SemanticName = "WEIGHT";
    inputElementDescs[3].SemanticIndex = 0;
    inputElementDescs[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;//float32_t4
    inputElementDescs[3].InputSlot = 1; // 1番目のslotのVBVのことだと伝える
    inputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[4].SemanticName = "INDEX";
    inputElementDescs[4].SemanticIndex = 0;
    inputElementDescs[4].Format = DXGI_FORMAT_R32G32B32A32_SINT; // int32_t4
    inputElementDescs[4].InputSlot = 1; // 1番目のslotのVBVのことだと伝える
    inputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    /*三角形を表示しよう*/

    ///InputLayoutの設定を行う

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs.data();
    inputLayoutDesc.NumElements = static_cast<UINT>(inputElementDescs.size());

    ///ShaderをCompileする

    //Shaderをコンパイルする
    Microsoft::WRL::ComPtr <IDxcBlob> object3DVSBlob = CompileShader(L"resources/shaders/Object3D.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(object3DVSBlob != nullptr);

    Microsoft::WRL::ComPtr <IDxcBlob> object3DPSBlob = CompileShader(L"resources/shaders/Object3D.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(object3DPSBlob != nullptr);

    Microsoft::WRL::ComPtr <IDxcBlob> fullscreenVSBlob = CompileShader(L"resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(fullscreenVSBlob != nullptr);

    Microsoft::WRL::ComPtr <IDxcBlob> fullscreenPSBlob = CompileShader(L"resources/shaders/CopyImage.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(fullscreenPSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> grayscalePSBlob = CompileShader(L"resources/shaders/Grayscale.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(grayscalePSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> sepiaPSBlob = CompileShader(L"resources/shaders/Sepia.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(sepiaPSBlob != nullptr);

    Microsoft::WRL::ComPtr <IDxcBlob> particleVSBlob = CompileShader(L"resources/shaders/Particle.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(particleVSBlob != nullptr);

    Microsoft::WRL::ComPtr <IDxcBlob> particlePSBlob = CompileShader(L"resources/shaders/Particle.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(particlePSBlob != nullptr);

    Microsoft::WRL::ComPtr <IDxcBlob> spriteVSBlob = CompileShader(L"resources/shaders/Object2D.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(spriteVSBlob != nullptr);

    Microsoft::WRL::ComPtr <IDxcBlob> spritePSBlob = CompileShader(L"resources/shaders/Object2D.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(spritePSBlob != nullptr);

    Microsoft::WRL::ComPtr <IDxcBlob> regionVSBlob = CompileShader(L"resources/shaders/Region.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(regionVSBlob != nullptr);

    Microsoft::WRL::ComPtr <IDxcBlob> byGeometryShaderVSBlob = CompileShader(L"resources/shaders/ByGeometryShader.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(byGeometryShaderVSBlob != nullptr);

    Microsoft::WRL::ComPtr <IDxcBlob> byGeometryShaderPSBlob = CompileShader(L"resources/shaders/ByGeometryShader.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(byGeometryShaderPSBlob != nullptr);

    Microsoft::WRL::ComPtr <IDxcBlob> byGeometryShaderGSBlob = CompileShader(L"resources/shaders/ByGeometryShader.GS.hlsl", L"gs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(byGeometryShaderGSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> lineVSBlob = CompileShader(L"resources/shaders/Line.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(lineVSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> linePSBlob = CompileShader(L"resources/shaders/Line.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(linePSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> lineInstancedVSBlob = CompileShader(L"resources/shaders/LineInstanced.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(lineInstancedVSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> lineInstancedPSBlob = CompileShader(L"resources/shaders/LineInstanced.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(lineInstancedPSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> skinningObject3DVSBlob = CompileShader(L"resources/shaders/SkinningObject3D.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(skinningObject3DVSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> skyboxVSBlob = CompileShader(L"resources/shaders/Skybox.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(skyboxVSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> skyboxPSBlob = CompileShader(L"resources/shaders/Skybox.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(skyboxPSBlob != nullptr);

    // Compute Shaderのコンパイル
    Microsoft::WRL::ComPtr<IDxcBlob> skinningCSBlob = CompileShader(L"resources/shaders/Skinning.CS.hlsl", L"cs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(skinningCSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> gpuParticleInitializeCSBlob = CompileShader(L"resources/shaders/InitializeParticle.CS.hlsl", L"cs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(gpuParticleInitializeCSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> gpuParticleEmitCSBlob = CompileShader(L"resources/shaders/EmitParticle.CS.hlsl", L"cs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(gpuParticleEmitCSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> gpuParticleUpdateCSBlob = CompileShader(L"resources/shaders/UpdateParticle.CS.hlsl", L"cs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(gpuParticleUpdateCSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> gpuParticleVSBlob = CompileShader(L"resources/shaders/ParticleGPU.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(gpuParticleVSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> gpuParticlePSBlob = CompileShader(L"resources/shaders/ParticleGPU.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(gpuParticlePSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> voxelParticleInitializeCSBlob = CompileShader(L"resources/shaders/InitializeVoxel.CS.hlsl", L"cs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(voxelParticleInitializeCSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> voxelParticleEmitCSBlob = CompileShader(L"resources/shaders/EmitVoxel.CS.hlsl", L"cs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(voxelParticleEmitCSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> voxelParticleUpdateCSBlob = CompileShader(L"resources/shaders/UpdateVoxel.CS.hlsl", L"cs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(voxelParticleUpdateCSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> voxelParticleVSBlob_ = CompileShader(L"resources/shaders/VoxelParticle.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(voxelParticleVSBlob_ != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> voxelParticlePSBlob_ = CompileShader(L"resources/shaders/VoxelParticle.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(voxelParticlePSBlob_ != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> vignettePSBlob = CompileShader(L"resources/shaders/Vignette.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(vignettePSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> smoothingPSBlob = CompileShader(L"resources/shaders/Smoothing.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(smoothingPSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> gaussianFilterPSBlob = CompileShader(L"resources/shaders/GaussianFilter.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(gaussianFilterPSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> depthBasedOutlinePSBlob = CompileShader(L"resources/shaders/DepthBasedOutline.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(depthBasedOutlinePSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> radialBlurPSBlob = CompileShader(L"resources/shaders/RadialBlur.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(radialBlurPSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> dissolvePSBlob = CompileShader(L"resources/shaders/Dissolve.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(dissolvePSBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> noisePSBlob = CompileShader(L"resources/shaders/Noise.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), log_->GetLogStream());
    assert(noisePSBlob != nullptr);


    // コンパイルが完了したのでdxcUtils、dxcCompiler、includeHandlerを解放
    if (dxcUtils) { dxcUtils.Reset(); }
    if (dxcCompiler) { dxcCompiler.Reset(); }
    if (includeHandler) { includeHandler.Reset(); }

    /*三角形を表示しよう*/

    ///PSOを生成する

    psoManager_ = std::make_unique<PSOManager>();

    PSOManager::ShaderSet objectShaders{
        object3DVSBlob,
        object3DPSBlob
    };

    PSOManager::ShaderSet particleShaders{
        particleVSBlob,
        particlePSBlob
    };

    PSOManager::ShaderSet spriteShaders{
        spriteVSBlob,
        spritePSBlob
    };

    PSOManager::ShaderSet regionShaders{
        regionVSBlob,
        object3DPSBlob   // PS は既存の Object3D.PS を流用
    };

    PSOManager::ShaderSet byGeometryShaders{
        byGeometryShaderVSBlob,
        byGeometryShaderPSBlob,
        byGeometryShaderGSBlob
    };

    PSOManager::ShaderSet lineShaders{
        lineVSBlob,
        linePSBlob
    };

    PSOManager::ShaderSet lineInstancedShaders{
        lineInstancedVSBlob,
        lineInstancedPSBlob
    };

    PSOManager::ShaderSet skinningObject3DShaders{
        skinningObject3DVSBlob,
        object3DPSBlob
    };

    PSOManager::ShaderSet skyboxShaders{
        skyboxVSBlob,
        skyboxPSBlob
    };

    PSOManager::ShaderSet gpuParticleShaders{
    gpuParticleVSBlob,
    gpuParticlePSBlob
    };

    PSOManager::ShaderSet voxelParticleShaders{
        voxelParticleVSBlob_,
        voxelParticlePSBlob_,
    };

    // 入力レイアウトは既存の inputLayoutDesc
    psoManager_->Initialize(
        device_.Get(),
        rootSignature_.Get(),
        inputLayoutDesc,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        objectShaders,
        particleShaders,
        spriteShaders,
        regionShaders,
        byGeometryShaders,
        lineShaders,
        lineInstancedShaders,
        skinningObject3DShaders,
        skyboxShaders,
        gpuParticleShaders,
        voxelParticleShaders
    );

    //実際に生成
    // 不透明(深度書き込みあり)
    psoManager_->Get(BlendMode::kBlendModeNone, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back);

    // Compute PSOの生成
    D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc{};
    computePsoDesc.pRootSignature = computeRootSignature_.Get();
    computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    computePsoDesc.CS = { skinningCSBlob->GetBufferPointer(), skinningCSBlob->GetBufferSize() };
    hr = device_->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&skinningComputePSO_));
    assert(SUCCEEDED(hr));

    computePsoDesc.CS = { gpuParticleInitializeCSBlob->GetBufferPointer(), gpuParticleInitializeCSBlob->GetBufferSize() };
    hr = device_->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&gpuParticleInitializePSO_));
    assert(SUCCEEDED(hr));

    computePsoDesc.CS = { gpuParticleEmitCSBlob->GetBufferPointer(), gpuParticleEmitCSBlob->GetBufferSize() };
    hr = device_->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&gpuParticleEmitPSO_));
    assert(SUCCEEDED(hr));

    computePsoDesc.CS = { gpuParticleUpdateCSBlob->GetBufferPointer(), gpuParticleUpdateCSBlob->GetBufferSize() };
    hr = device_->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&gpuParticleUpdatePSO_));
    assert(SUCCEEDED(hr));

    computePsoDesc.CS = { voxelParticleInitializeCSBlob->GetBufferPointer(), voxelParticleInitializeCSBlob->GetBufferSize() };
    hr = device_->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&voxelParticleInitializePSO_));
    assert(SUCCEEDED(hr));

    computePsoDesc.CS = { voxelParticleEmitCSBlob->GetBufferPointer(), voxelParticleEmitCSBlob->GetBufferSize() };
    hr = device_->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&voxelParticleEmitPSO_));
    assert(SUCCEEDED(hr));

    computePsoDesc.CS = { voxelParticleUpdateCSBlob->GetBufferPointer(), voxelParticleUpdateCSBlob->GetBufferSize() };
    hr = device_->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&voxelParticleUpdatePSO_));
    assert(SUCCEEDED(hr));
    assert(SUCCEEDED(hr));


    // 生成が完了したのでShaderBlobを解放
    if (object3DVSBlob) { object3DVSBlob.Reset(); }
    if (object3DPSBlob) { object3DPSBlob.Reset(); }
    if (particleVSBlob) { particleVSBlob.Reset(); }
    if (particlePSBlob) { particlePSBlob.Reset(); }
    if (spriteVSBlob) { spriteVSBlob.Reset(); }
    if (spritePSBlob) { spritePSBlob.Reset(); }
    if (regionVSBlob) { regionVSBlob.Reset(); }
    if (byGeometryShaderVSBlob) { byGeometryShaderVSBlob.Reset(); }
    if (byGeometryShaderPSBlob) { byGeometryShaderPSBlob.Reset(); }
    if (byGeometryShaderGSBlob) { byGeometryShaderGSBlob.Reset(); }
    if (lineVSBlob) { lineVSBlob.Reset(); }
    if (linePSBlob) { linePSBlob.Reset(); }
    if (lineInstancedVSBlob) { lineInstancedVSBlob.Reset(); }
    if (lineInstancedPSBlob) { lineInstancedPSBlob.Reset(); }
    if (skinningObject3DVSBlob) { skinningObject3DVSBlob.Reset(); }
    if (skyboxVSBlob) { skyboxVSBlob.Reset(); }
    if (skyboxPSBlob) { skyboxPSBlob.Reset(); }
    if (fullscreenVSBlob) { fullscreenVSBlob.Reset(); }
    if (fullscreenPSBlob) { fullscreenPSBlob.Reset(); }
    if (grayscalePSBlob) { grayscalePSBlob.Reset(); }
    if (sepiaPSBlob) { sepiaPSBlob.Reset(); }
    if (vignettePSBlob) { vignettePSBlob.Reset(); }
    if (smoothingPSBlob) { smoothingPSBlob.Reset(); }
    if (skinningCSBlob) { skinningCSBlob.Reset(); }
    if (gpuParticleInitializeCSBlob) { gpuParticleInitializeCSBlob.Reset(); }
    if (gpuParticleEmitCSBlob) { gpuParticleEmitCSBlob.Reset(); }
    if (gpuParticleUpdateCSBlob) { gpuParticleUpdateCSBlob.Reset(); }
    if (gpuParticleVSBlob) { gpuParticleVSBlob.Reset(); }
    if (gpuParticlePSBlob) { gpuParticlePSBlob.Reset(); }
    if (voxelParticleInitializeCSBlob) { voxelParticleInitializeCSBlob.Reset(); }
    // --- VoxelParticle Emit ---
    if (voxelParticleEmitCSBlob) { voxelParticleEmitCSBlob.Reset(); }
    // --- VoxelParticle Update ---
    if (voxelParticleUpdateCSBlob) { voxelParticleUpdateCSBlob.Reset(); }
    if (noisePSBlob) { noisePSBlob.Reset(); }


    //頂点リソース用のヒープを生成
    D3D12_HEAP_PROPERTIES uploadHeapProperties{};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; //UploadHeapを使う
    //頂点リソースの設定
    D3D12_RESOURCE_DESC vertexResourceDesc{};
    //バッファリソース、テクスチャの場合はまた別の設定をする
    vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

    /*テクスチャを貼ろう*/

    ///頂点データの拡張
    vertexResourceDesc.Width = sizeof(VertexData) * 3; //リソースのサイズ。今回はVertexDataを3頂点分

    /*三角形を表示しよう*/

    ///PSOを生成する

    //バッファの場合はこれらは1にする決まり
    vertexResourceDesc.Height = 1;
    vertexResourceDesc.DepthOrArraySize = 1;
    vertexResourceDesc.MipLevels = 1;
    vertexResourceDesc.SampleDesc.Count = 1;
    //バッファの場合はこれにする決まり
    vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    //実際に頂点リソースを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> dummyVertexResource = nullptr;
    hr = device_->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(dummyVertexResource.GetAddressOf()));
    assert(SUCCEEDED(hr));

    // 頂点リソースを作ったのでdummyVertexResourceを解放
    if (dummyVertexResource) { dummyVertexResource.Reset(); }

    /*三角形を表示しよう*/

    ///ViewportとScissor(シザー)

    //クライアント領域のサイズと一緒にして画面全体に表示
    viewport_.Width = static_cast<FLOAT>(clientWidth_);
    viewport_.Height = static_cast<FLOAT>(clientHeight_);
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    //基本的にビューポートと同じ矩形が構成されるようにする
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

    return GetCPUDescriptorHandle(rtvDescriptorHeap_, descriptorSizeRTV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetRTVGPUDescriptorHandle(uint32_t index) {

    return GetGPUDescriptorHandle(rtvDescriptorHeap_, descriptorSizeRTV, index);
}


D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetDSVCPUDescriptorHandle(uint32_t index) {

    return GetCPUDescriptorHandle(dsvDescriptorHeap_, descriptorSizeDSV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetDSVGPUDescriptorHandle(uint32_t index) {

    return GetGPUDescriptorHandle(dsvDescriptorHeap_, descriptorSizeDSV, index);
}

uint32_t DirectXCommon::AllocateRTVIndex() {
    assert(nextRtvIndex_ < 32);
    return nextRtvIndex_++;
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

/*テクスチャを正しく配置しよう*/

[[nodiscard]] //戻り値を破棄しないように
Microsoft::WRL::ComPtr<ID3D12Resource>  DirectXCommon::UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages) {
    ///IntermediateResource(中間リソース)

    std::vector<D3D12_SUBRESOURCE_DATA> subResources;
    //1. PrepareUploadを利用して、読み込んだデータからDirectX12用のSubResource(サブリソース)の配列を作成する(SubResourceは、MipMapの1枚1枚ぐらいのイメージでいると良い)
    DirectX::PrepareUpload(device_.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subResources);
    //2. SubResourceの数を基に、コピー元となるIntermediateResourceに必要なサイズを計算する
    uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subResources.size()));
    //3. 計算したサイズでIntermediateResourceを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(intermediateSize);

    ///データ転送をコマンドに積む

    UpdateSubresources(commandList_.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subResources.size()), subResources.data());

    ///ResourceStateを変更し、IntermediateResourceを返す

    //Textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    commandList_->ResourceBarrier(1, &barrier);

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

    if (!std::filesystem::exists(filePathW)) {
        std::wstring msg = L"[LoadTexture] File not found: " + filePathW + L"\n";
        OutputDebugStringW(msg.c_str());
        assert(false && "Texture file not found");
    }

    // --- sRGB 判定ロジック ---
    // 【リニアワークフロー対応】
    // 画像が「色データ」か「数値データ」かをファイル名から判別します。
    // - 色データ (BaseColor等): sRGB→Linear変換が必要
    // - 数値データ (Normal/AO等): 変換すると値が壊れるためそのまま読み込む
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
    if (StringUtility::EndsWith(filePathW, L".dds")) {
        hr = LoadFromDDSFile(filePathW.c_str(), DDS_FLAGS_NONE, nullptr, image);
    }
    else {
        // カラーテクスチャなら FORCE_SRGB でリニアライズ
        WIC_FLAGS wicFlags = isSRGB ? WIC_FLAGS_FORCE_SRGB : WIC_FLAGS_NONE;
        hr = LoadFromWICFile(filePathW.c_str(), wicFlags, nullptr, image);
    }

    if (FAILED(hr)) {
        _com_error err(hr);
        std::wstring msg = L"[LoadTexture] Load failed (" + std::to_wstring(hr) +
            L"): " + filePathW + L" - " + err.ErrorMessage() + L"\n";
        OutputDebugStringW(msg.c_str());
        assert(false && "LoadTexture failed");
    }

    ScratchImage mipImages{};
    if (IsCompressed(image.GetMetadata().format)) {
        mipImages = std::move(image);
    }
    else {
        // ミップマップ生成時も sRGB 用フィルタを使い分ける
        TEX_FILTER_FLAGS filter = isSRGB ? TEX_FILTER_SRGB : TEX_FILTER_DEFAULT;
        hr = GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
            filter, 0, mipImages);
        if (FAILED(hr)) {
            _com_error err(hr);
            std::wstring msg = L"[LoadTexture] GenerateMipMaps failed (" + std::to_wstring(hr) + L")\n";
            OutputDebugStringW(msg.c_str());
            assert(false && "GenerateMipMaps failed");
        }
    }

    return mipImages;
}

// FPS固定初期化
void DirectXCommon::InitializeFixFPS() {
    // 現在時間を記録する
    reference_ = std::chrono::steady_clock::now();
}

// FPS固定更新
void DirectXCommon::UpdateFixFPS() {

    // 1/60秒ぴったりの時間
    const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
    // 1/60秒よりわずかに短い時間
    const std::chrono::microseconds kMinChecktime(uint64_t(1000000.0f / 65.0f));

    // 現在時間を取得する(VSync待ちが完了した時点での時間を取得する)
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    // 前回記録からの経過時間を取得する(現在時間から前回の時間を引くことで、前回からの経過時間を計算する)
    std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

    // 1/60秒(よりわずかに短い時間)経っていない場合
    if (elapsed < kMinChecktime) {
        // 1/60秒経過するまで微小なスリープを繰り返す(前回から1/60秒経つまで待機する)
        while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
            // 1マイクロ秒スリープ
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }
    // 現在の時間を記録する(次フレームの前回記録からの経過時間を取得の計算に使うため、待機完了後の時間を記録しておく)
    reference_ = std::chrono::steady_clock::now();

}

Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::CompileShader(
    //CompilerするShaderファイルへのパス
    const std::wstring& filePath,
    //Compilerに使用するProfile
    const wchar_t* profile,
    //初期化で生成したものを3つ
    const Microsoft::WRL::ComPtr<IDxcUtils>& dxcUtils,
    const Microsoft::WRL::ComPtr<IDxcCompiler3>& dxcCompiler,
    const Microsoft::WRL::ComPtr<IDxcIncludeHandler>& includeHandler,
    std::ostream& os
) {

    /// 1. hlslファイルを読む

    //これからシェーダーをコンパイルする旨をログに出す
    Log::OutPutLog(os, ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));
    //hislファイルを読む
    IDxcBlobEncoding* shaderSource = nullptr;
    HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    //読めなかったら止める
    assert(SUCCEEDED(hr));
    //読み込んだファイルの内容を設定する
    DxcBuffer shaderSourceBuffer;
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8; //UTF8の文字コードであることを通知

    /// 2. Compileする

    LPCWSTR arguments[]{
        filePath.c_str(), // コンパイル対象のhlslファイル名
        L"-E",L"main", // エントリーポイントの指定。基本的にmain以外にはしない
        L"-T",profile, // ShaderProfileの設定
        L"-Zi",L"-Qembed_debug", // デバッグ用の情報を埋め込む
        L"-Od", // 最適化を外しておく
        L"-Zpr", // メモリレイアウトは行優先
    };
    //実際にShaderをコンパイルする
    IDxcResult* shaderResult = nullptr;
    hr = dxcCompiler->Compile(
        &shaderSourceBuffer, // 読み込んだファイル
        arguments, // コンパイルオプション
        _countof(arguments), // コンパイルオプションの数
        includeHandler.Get(), // includeが含まれた諸々
        IID_PPV_ARGS(&shaderResult) // コンパイル結果
    );
    //コンパイルエラーではなくdxcが起動できないなど致命的な状況
    assert(SUCCEEDED(hr));

    /// 3. 警告・エラーが出ていないか確認する

    //警告・エラーが出ていたらログに出して止める
    IDxcBlobUtf8* shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
        Log::OutPutLog(os, shaderError->GetStringPointer());
        //警告・エラーダメゼッタイ
        assert(false);
    }

    /// 4. Compile結果を受け取って返す

    //コンパイル結果から実行用のバイナリ部分を取得
    IDxcBlob* shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));
    //成功したらログを出す
    Log::OutPutLog(os, ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));
    //もう使わないリソースを解散
    shaderSource->Release();
    shaderResult->Release();
    //実行用のバイナリを返却
    return shaderBlob;

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
    fenceValue_++;
    commandQueue_->Signal(fence_.Get(), fenceValue_);
    if (fence_->GetCompletedValue() < fenceValue_) {
        fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
        WaitForSingleObject(fenceEvent_, INFINITE);
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
        device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc_, rtvHandles_[i]);
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
