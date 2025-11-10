#pragma once

#include <xaudio2.h>
#include <wrl.h>

struct SoundData {
    //波形フォーマット
    WAVEFORMATEX wfex;
    //バッファの先頭アドレス
    BYTE* pBuffer;
    //バッファのサイズ
    unsigned int bufferSize;
};