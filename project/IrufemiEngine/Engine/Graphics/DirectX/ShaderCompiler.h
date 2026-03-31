#pragma once

#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>
#include <string>
#include <ostream>

/**
 * @class ShaderCompiler
 * @brief DXC (DirectX Shader Compiler) を使用してシェーダをコンパイルするクラス
 */
class ShaderCompiler {
public:
    /**
     * @brief 初期化
     */
    void Initialize();

    /**
     * @brief シェーダのコンパイル
     * @param[in] filePath hlslファイルへのパス
     * @param[in] profile コンパイルプロファイル (vs_6_0, ps_6_0等)
     * @param[in] os ログ出力用ストリーム
     * @return コンパイルされたシェーダのBlob
     */
    Microsoft::WRL::ComPtr<IDxcBlob> Compile(
        const std::wstring& filePath,
        const wchar_t* profile,
        std::ostream& os
    );

private:
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
};
