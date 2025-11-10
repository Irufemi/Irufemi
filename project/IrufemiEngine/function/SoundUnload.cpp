#include "Function.h"

#include "../math/SoundData.h"

///音声データの解放

//音声データ解放
void SoundUnload(SoundData* soundData) {
    //バッファのメモリを解放
    delete[] soundData->pBuffer;

    soundData->pBuffer = 0;
    soundData->bufferSize = 0;
    soundData->wfex = {};
}