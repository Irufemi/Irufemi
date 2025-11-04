#pragma once

#include "../manager/AudioManager.h"
#include <string>
#include <memory>

class Se {
private:
    IXAudio2SourceVoice* voice_ = nullptr;
    float volume_ = 0.6f; // SEは通常フルボリュームを想定
    std::shared_ptr<Sound> sound_;
    std::string soundKey_;

    // AudioManager は外部で一度セットする（Bgm と同様）
    static AudioManager* audioManager_;

public:
    Se() = default;
    ~Se();

    // audioManager は起動時に SetAudioManager でセットすること
    // filePath をロードして内部に保持。key を空にすると filePath がキーとして登録される
    // loop のデフォルトは false（初期設定でループ無し）
    void Initialize(const std::string& filePath, const std::string& key = "", float volume = 0.6f, bool loop = false, bool autoPlay = false);

    // ランタイムで差し替え（ファイル／キー）
    bool SetSourceByFile(const std::string& filePath, const std::string& key = "", float volume = 1.0f);
    bool SetSourceByKey(const std::string& key, float volume = 1.0f);

    // 再生（デフォルトで loop = false）
    void Play(bool loop = false);

    // 停止・解放
    void Stop();

    void SetVolume(float volume);
    float GetVolume() const { return volume_; }

    // グローバルに AudioManager をセットする
    static void SetAudioManager(AudioManager* mgr) { audioManager_ = mgr; }
};

