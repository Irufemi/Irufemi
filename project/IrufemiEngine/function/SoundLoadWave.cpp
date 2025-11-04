#include "../math/SoundData.h"
#include "../math/RiffHeader.h"
#include "../math/FormatChunk.h"
#include <fstream>
#include <cassert>


/*サウンド再生*/

///音声データの読み込み

SoundData SoundLoadWave(const char* filename) {

    ///①ファイルオープン
    std::ifstream file;

    file.open(filename, std::ios_base::binary);

    assert(file.is_open());

    ///②wavデータ読み込み

    RiffHeader riff;
    file.read((char*)&riff, sizeof(riff));

    if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
        assert(0);
    }
    //タイプがWAVEかチェック
    if (strncmp(riff.type, "WAVE", 4) != 0) {
        assert(0);
    }

    //Formatチャンクの読み込み
    FormatChunk format = {};
    //チャンクヘッダーの確認
    file.read((char*)&format, sizeof(ChunkHeader));
    if (strncmp(format.chunk.id, "fmt", 4) != 0) {
        assert(0);
    }

    //チャンク本体の読み込み
    assert(format.chunk.size <= sizeof(format.fmt));

    file.read((char*)&format.fmt, format.chunk.size);

    //Dataチャンクの読み込み
    ChunkHeader data;
    file.read((char*)&data, sizeof(data));
    //JUNKチャンクを検出した場合
    if (strncmp(data.id, "JUNK", 4) == 0) {
        //読み取り位置をJUNKチャンクの終わりまで進める
        file.seekg(data.size, std::ios_base::cur);
        //再読み込み
        file.read((char*)&data, sizeof(data));
    }


    if (strncmp(data.id, "data", 4) != 0) {
        assert(0);
    }

    //Dataチャンクのデータ部(波形データ)の読み込み
    char* pBuffer = new char[data.size];
    file.read(pBuffer, data.size);

    //Waveファイルを閉じる
    file.close();

    ///④読み込んだ音声データをreturn

    ///returnするための音声データ
    SoundData soundData = {};

    soundData.wfex = format.fmt;
    soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
    soundData.bufferSize = data.size;

    return soundData;
}