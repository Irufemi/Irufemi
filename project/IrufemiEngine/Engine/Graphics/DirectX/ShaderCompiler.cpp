#include "ShaderCompiler.h"
#include "../../Core/Utility/Log.h"
#include "../../Core/Utility/StringUtility.h"
#include <format>
#include <cassert>
#include <format>

/**
 * @brief 初期化
 */
void ShaderCompiler::Initialize() {
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils_.GetAddressOf()));
    assert(SUCCEEDED(hr));
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(dxcCompiler_.GetAddressOf()));
    assert(SUCCEEDED(hr));
    hr = dxcUtils_->CreateDefaultIncludeHandler(includeHandler_.GetAddressOf());
    assert(SUCCEEDED(hr));
}

/**
 * @brief シェーダのコンパイル
 * @param[in] filePath hlslファイルへのパス
 * @param[in] profile コンパイルプロファイル
 * @param[in] os ログ出力用ストリーム
 * @return コンパイルされたシェーダのBlob
 */
Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::Compile(
    const std::wstring& filePath,
    const wchar_t* profile,
    std::ostream& os
) {
    Log::OutPutLog(os, ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));

    // hlslファイルを読む
    IDxcBlobEncoding* shaderSource = nullptr;
    HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    assert(SUCCEEDED(hr));

    DxcBuffer shaderSourceBuffer;
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8;

    // Compile引数
    LPCWSTR arguments[] = {
        filePath.c_str(),
        L"-E", L"main",
        L"-T", profile,
        L"-Zi", L"-Qembed_debug",
        L"-Od",
        L"-Zpr",
    };

    // 実際にコンパイル
    IDxcResult* shaderResult = nullptr;
    hr = dxcCompiler_->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler_.Get(),
        IID_PPV_ARGS(&shaderResult)
    );
    assert(SUCCEEDED(hr));

    // エラー・警告の確認
    IDxcBlobUtf8* shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
        Log::OutPutLog(os, shaderError->GetStringPointer());
        assert(false && "Shader Compile Error");
    }

    // 結果の取得
    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderBlob.GetAddressOf()), nullptr);
    assert(SUCCEEDED(hr));

    Log::OutPutLog(os, ConvertString(std::format(L"CompileShader Success, path:{}\n", filePath)));

    // リソースの解放
    shaderSource->Release();
    shaderResult->Release();

    return shaderBlob;
}
