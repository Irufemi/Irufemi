#include "AudioSourceComponent.h"
#include "../GameObject.h"
#include "../BaseScene.h"
#include "Engine/IrufemiEngine.h"
#include "Resource/Audio/AudioManager.h"

AudioSourceComponent::AudioSourceComponent() {
}

AudioSourceComponent::~AudioSourceComponent() {
    Stop();
}

void AudioSourceComponent::OnRegisterProperties() {
    RegisterProperty("Audio Type (0:BGM, 1:SE)", &audioType_);
    RegisterProperty("Audio Path", &audioPath_);
    RegisterProperty("Play On Awake", &playOnAwake_);
    RegisterProperty("Loop", &loop_);
    RegisterProperty("Volume", &volume_);
}

void AudioSourceComponent::Initialize() {
    if (!gameObject_) return;
    auto scene = gameObject_->GetScene();
    if (!scene) return;
    auto engine = scene->GetEngine();
    if (!engine) return;

    auto audioManager = engine->GetAudioManager();
    if (audioManager) {
        player_ = std::make_unique<AudioPlayer>(audioManager, static_cast<AudioType>(audioType_));
        
        std::string fullPath = "resources/" + audioPath_;
        if (audioPath_.find("resources/") == 0) {
            fullPath = audioPath_;
        }
        player_->Initialize(fullPath);
        player_->SetVolume(volume_);
    }

    if (playOnAwake_) {
        Play();
    }
}

void AudioSourceComponent::Update() {
    // インスペクターからの動的変更を反映（EditorMode等での調整を想定）
    if (player_) {
        player_->SetVolume(volume_);
    }
}

void AudioSourceComponent::Play() {
    if (player_) {
        player_->Play(loop_);
    }
}

void AudioSourceComponent::Stop() {
    if (player_) {
        player_->Stop();
    }
}

void AudioSourceComponent::SetAudioPath(const std::string& path) {
    audioPath_ = path;
    if (player_) {
        std::string fullPath = "resources/" + audioPath_;
        if (audioPath_.find("resources/") == 0) {
            fullPath = audioPath_;
        }
        player_->Initialize(fullPath);
    }
}

void AudioSourceComponent::SetVolume(float volume) {
    volume_ = volume;
    if (player_) {
        player_->SetVolume(volume);
    }
}

void AudioSourceComponent::SetLoop(bool loop) {
    loop_ = loop;
}

void AudioSourceComponent::SetAudioType(AudioType type) {
    audioType_ = static_cast<int>(type);
    if (player_) {
        player_->SetAudioType(type);
    }
}
