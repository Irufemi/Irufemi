#pragma once

#include "../source/Sound.h" // 音声データクラス
#include <map>
#include <memory>
#include <string>
#include <vector>

// IXAudio2SourceVoice構造体を前方宣言（ヘッダの依存関係を減らすため）
struct IXAudio2SourceVoice;

class AudioManager {
private:
    // コピー禁止
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // XAudio2のコアインターフェース
    IXAudio2* pXAudio2_{ nullptr };
    IXAudio2MasteringVoice* pMasteringVoice_{ nullptr };

    // ロードした音声データをファイル名をキーにして保持するマップ
    std::map<std::string, std::shared_ptr<Sound>> soundRegistry_;

    // 再生中の SourceVoice を一元管理
    std::vector<IXAudio2SourceVoice*> activeVoices_;

    // カテゴリ名 → その中にあるファイル名リスト（ソート済み）
    std::map<std::string, std::vector<std::string>> categoryMap_;

    // 追加: ファイナライズ済みフラグ
    bool finalized_{ false };

    // 追加: 管理対象か判定
    bool IsManagedVoice(IXAudio2SourceVoice* voice) const;

public:
    // コンストラクタ
    AudioManager() = default;
    // デストラクタ
    ~AudioManager();

    // オーディオエンジンの初期化・終了処理
    void Initialize();
    void Finalize();

    //Media Foundationの初期化
    void StartUp();

    // 指定フォルダから対応する音声ファイルをすべてロードする
    void LoadAllSoundsFromFolder(const std::string& folderPath);

    // サブフォルダ単位でロードするオーバーロード版
    void LoadSoundsFromFolder(const std::string& folderPath, const std::string& category);

    // カテゴリ（サブフォルダ）単位でソート済みのサウンド名一覧を取得する
    std::vector<std::string> GetSoundNames(const std::string& category) const;

    // 名前を指定してロード済みのサウンドデータを取得する
    std::shared_ptr<Sound> GetSoundData(const std::string& name) const;

    // 利用可能なサウンドカテゴリ一覧（サブフォルダ名）を取得
    std::vector<std::string> GetCategories() const;

    // サウンドを再生し、再生中のボイスへのポインタを返す
    IXAudio2SourceVoice* Play(
        std::shared_ptr<Sound> soundData, bool loop = false, float volume = 1.0f);

    // 再生中のボイスを停止し、破棄する
    void Stop(IXAudio2SourceVoice*& voice);
    void StopAll(); // 追加: 全Voice停止（Finalize内で利用）

    // 追加: 重複を避けてファイルからロード or 取得（TextureManager風）
    // key が空なら filePath をキーにする
    std::shared_ptr<Sound> GetOrLoadSoundByFile(const std::string& filePath, const std::string& key = "");

    // 追加: 指定キーを保持しているか
    bool HasSound(const std::string& key) const;
};