#pragma once

/*DirectX12を初期化しよう*/

#include <d3d12.h>

/*三角形を表示しよう*/

#include <dxcapi.h>

/*objファイルを読んでみよう*/

#include "../math/ModelData.h"
#include "../math/MaterialData.h"

#include <wrl.h>

#include "../math/ObjModel.h"


/*サウンド再生*/
#include "../math/SoundData.h"
#include <xaudio2.h> 

/*ログを出そう*/

LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

/*三角形を表示しよう*/

IDxcBlob* CompileShader(
    //CompilerするShaderファイルへのパス
    const std::wstring& filePath,
    //Compilerに使用するProfile
    const wchar_t* profile,
    //初期化で生成したものを3つ
    const Microsoft::WRL::ComPtr<IDxcUtils>& dxcUtils,
    const Microsoft::WRL::ComPtr<IDxcCompiler3>& dxcCompiler,
    const Microsoft::WRL::ComPtr<IDxcIncludeHandler>& includeHandler,
    std::ostream& os
);

/*前後関係を正しくしよう*/

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height);

/*objjファイルを読んでみよう*/

///　ModelData構造体と読み込み関数

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

ObjModel LoadObjFileM(const std::string& directoryPath, const std::string& filename);

ModelData LoadObjFileAssimp(const std::string& directoryPath, const std::string& filename);

ObjModel LoadObjFileAssimpM(const std::string& directoryPath, const std::string& filename);

// f行の頂点データを安全にパースする関数例
bool ParseObjFaceToken(const std::string& token, int& posIdx, int& uvIdx, int& normIdx);

/// MaterialData構造体と読み込み関数

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string filename);

/*サウンド再生*/

SoundData SoundLoadWave(const char* filename);

///音声データの解放

//音声データ解放
void SoundUnload(SoundData* soundData);

///サウンドの再生

//音声再生
void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData);