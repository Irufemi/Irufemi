#include <xaudio2.h>
#include <cassert>
#include "../math/SoundData.h"

/*サウンド再生*/

///サウンドの再生

//音声再生
void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData) {

    HRESULT result;

    //波形フォーマットを元にSourceVoiceの作成
    IXAudio2SourceVoice* pSourceVoice = nullptr;
    result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
    assert(SUCCEEDED(result));

    //再生する波形データの設定
    XAUDIO2_BUFFER buf;
    buf.pAudioData = soundData.pBuffer;
    buf.AudioBytes = soundData.bufferSize;
    buf.Flags = XAUDIO2_END_OF_STREAM;

    //波形データの再生
    result = pSourceVoice->SubmitSourceBuffer(&buf);
    result = pSourceVoice->Start();

}