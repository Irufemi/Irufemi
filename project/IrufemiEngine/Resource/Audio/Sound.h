#pragma once
#include <vector>
#include <string>
#include <xaudio2.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h> // ComPtr用

#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

/**
 * @class Sound
 * @brief メモリ上にロードされたオーディオデータ（PCM）を管理するクラス
 * @details Media Foundation を用いて音声ファイルからPCMデータを読み込み、保持します。
 */
class Sound {
public:
    Sound() = default;
    ~Sound();

    /**
     * @brief ファイルから音声データを読み込む
     * @param[in] filePath 読み込むファイルのパス (WAV, MP3等)
     * @return 読み込み成功時に true
     */
    bool Load(const std::wstring& filePath);

    /**
     * @brief WAVEFORMATEX構造体へのポインタを取得する
     * @return WAVEFORMATEX* 読み込んだ音声のフォーマット情報
     */
    const WAVEFORMATEX* GetFormat() const { return pWaveFormat_; }

    /**
     * @brief オーディオデータの先頭ポインタを取得する
     * @return const BYTE* オーディオデータ配列
     */
    const BYTE* GetData() const { return mediaData_.data(); }

    /**
     * @brief オーディオデータのサイズ（バイト数）を取得する
     * @return UINT32 データサイズ
     */
    UINT32 GetSize() const { return static_cast<UINT32>(mediaData_.size()); }

private:
    WAVEFORMATEX* pWaveFormat_{ nullptr }; ///< 波形フォーマット情報。CoTaskMemFreeで解放が必要
    std::vector<BYTE> mediaData_;          ///< 読み込まれたオーディオデータ本体
};