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

    /**
     * @brief Initialize を実行する。
     */
    void Initialize() override;
    /**
     * @brief Update を実行する。
     */
    void Update() override;
    
    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override { return "AudioSourceComponent"; }
    /**
     * @brief OnRegisterProperties を実行する。
     */
    void OnRegisterProperties() override;

    /** @brief 音声を再生する */
    void Play();
    /** @brief 音声を停止する */
    void Stop();

    /**
     * @brief AudioPath を設定する。
     * @param[in] path 設定する AudioPath の値
     */
    void SetAudioPath(const std::string& path);
    /**
     * @brief Volume を設定する。
     * @param[in] volume 設定する Volume の値
     */
    void SetVolume(float volume);
    /**
     * @brief Loop を設定する。
     * @param[in] loop 設定する Loop の値
     */
    void SetLoop(bool loop);
    /**
     * @brief AudioType を設定する。
     * @param[in] type 設定する AudioType の値
     */
    void SetAudioType(AudioType type);

private:
    std::string audioPath_ = "audio/BGM/bgm_default.wav"; // デフォルト
    int audioType_ = static_cast<int>(AudioType::SE); // シリアライズ用
    bool playOnAwake_ = false;
    bool loop_ = false;
    float volume_ = 1.0f;
    
    std::unique_ptr<AudioPlayer> player_; ///< 実際の再生を担うObject層のインスタンス
};
