#pragma once
#include "../Component.h"
#include <string>
#include <memory>
#include "Resource/Audio/AudioPlayer.h"

/**
 * @class AudioSourceComponent
 * @brief 音声再生（BGM/SE）を管理するコンポーネント
 * @details 描画システムのMeshRendererComponent等と同様に、内部でAudioPlayer(Object層)をカプセル化します。
 */
class AudioSourceComponent : public Component {
public:
    AudioSourceComponent();
    ~AudioSourceComponent() override;

    void Initialize() override;
    void Update() override;
    
    std::string GetComponentName() const override { return "AudioSourceComponent"; }
    void OnRegisterProperties() override;

    /** @brief 音声を再生する */
    void Play();
    /** @brief 音声を停止する */
    void Stop();

    void SetAudioPath(const std::string& path);
    void SetVolume(float volume);
    void SetLoop(bool loop);
    void SetAudioType(AudioType type);

private:
    std::string audioPath_ = "audio/BGM/bgm_default.wav"; // デフォルト
    int audioType_ = static_cast<int>(AudioType::SE); // シリアライズ用
    bool playOnAwake_ = false;
    bool loop_ = false;
    float volume_ = 1.0f;
    
    std::unique_ptr<AudioPlayer> player_; ///< 実際の再生を担うObject層のインスタンス
};
