#include "Bgm.h"

#include <algorithm>
#include <imgui.h>

AudioManager* Bgm::audioManager_ = nullptr;

Bgm::~Bgm() {
    Stop();
    currentSound_.reset();
}

void Bgm::Initialize(const std::string& filePath, const std::string& key, bool loop, bool autoPlay) {
    // 以前の音源をクリアしてからセット
    Stop();
    currentSound_.reset();
    soundKey_.clear();
    fixedLoop_ = true;

    if (!audioManager_) return; // 事前に Bgm::SetAudioManager を呼ぶこと

    auto sound = audioManager_->GetOrLoadSoundByFile(filePath, key);
    if (!sound) {
        // 読み込み失敗
        return;
    }

    // currentSound_ を保持（これが存在する = 固定モード）
    currentSound_ = sound;
    soundKey_ = key.empty() ? filePath : key;
    fixedLoop_ = loop;

    if (autoPlay) {
        PlayFixed();
    }
}

bool Bgm::SetSourceByFile(const std::string& filePath, const std::string& key, bool loop, bool autoPlay) {
    if (!audioManager_) return false;
    auto sound = audioManager_->GetOrLoadSoundByFile(filePath, key);
    if (!sound) return false;

    Stop();
    currentSound_ = sound;
    soundKey_ = key.empty() ? filePath : key;
    fixedLoop_ = loop;

    if (autoPlay) PlayFixed();
    return true;
}

bool Bgm::SetSourceByKey(const std::string& key, bool loop, bool autoPlay) {
    if (!audioManager_) return false;
    auto sound = audioManager_->GetSoundData(key);
    if (!sound) return false;

    Stop();
    currentSound_ = sound;
    soundKey_ = key;
    fixedLoop_ = loop;

    if (autoPlay) PlayFixed();
    return true;
}

void Bgm::ClearSource() {
    Stop();
    currentSound_.reset();
    soundKey_.clear();
    fixedLoop_ = true;
}

void Bgm::Play(const std::string& category, const std::string& track, bool loop) {
    if (!audioManager_) return;
    Stop();
    std::string key = category + "/" + track;
    auto sound = audioManager_->GetSoundData(key);
    if (sound) {
        voice_ = audioManager_->Play(sound, loop, volume_);
    }
}

void Bgm::PlayFixed() {
    if (!audioManager_ || !currentSound_) return;
    Stop();
    voice_ = audioManager_->Play(currentSound_, fixedLoop_, volume_);
}

void Bgm::PlayFirstTrack() {
    if (!audioManager_) return; // 保険
    auto cats = audioManager_->GetCategories();
    if (!cats.empty()) {
        selectedCat_ = 0;
        auto tracks = audioManager_->GetSoundNames(cats[selectedCat_]);
        if (!tracks.empty()) {
            selectedTrack_ = 0;
            Play(cats[selectedCat_], tracks[selectedTrack_], true);
        }
    }
}

void Bgm::Stop() {
    if (voice_) {
        if (audioManager_) {
            audioManager_->Stop(voice_);
        } else {
            // audioManager_ がない場合は最低限 Stop を試みる（安全策）
            voice_->Stop(0);
            voice_->DestroyVoice();
            voice_ = nullptr;
        }
        voice_ = nullptr;
    }
}

void Bgm::SetVolume(float volume) {
    volume_ = volume;
    if (voice_) {
        voice_->SetVolume(volume_);
    }
}

void Bgm::Update() {
#if defined(_DEBUG) || defined(DEVELOPMENT)

    ImGui::Begin("Audio Settings");

    if (currentSound_) {
        // currentSound_ が存在する -> 固定再生モードとみなす
        const char* label = soundKey_.empty() ? "(custom)" : soundKey_.c_str();
        ImGui::Text("BGM (Fixed): %s", label);
        bool loop = fixedLoop_;
        if (ImGui::Checkbox("Loop", &loop)) {
            fixedLoop_ = loop;
            if (voice_) {
                // ループ変更を反映するため再生し直し
                PlayFixed();
            }
        }
    } else {
        // 従来のカテゴリ/トラック選択UI
        if (!audioManager_) {
            ImGui::Text("AudioManager not set");
        } else {
            auto cats = audioManager_->GetCategories();
            if (!cats.empty()) {
                selectedCat_ = std::clamp(selectedCat_, 0, (int)cats.size() - 1);
                if (ImGui::BeginCombo("Category", cats[selectedCat_].c_str())) {
                    for (int i = 0; i < (int)cats.size(); ++i) {
                        bool sel = (i == selectedCat_);
                        if (ImGui::Selectable(cats[i].c_str(), sel)) {
                            selectedCat_ = i;
                            selectedTrack_ = 0;
                        }
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                auto tracks = audioManager_->GetSoundNames(cats[selectedCat_]);
                if (!tracks.empty()) {
                    selectedTrack_ = std::clamp(selectedTrack_, 0, (int)tracks.size() - 1);
                    if (ImGui::BeginCombo("BGM Track", tracks[selectedTrack_].c_str())) {
                        for (int i = 0; i < (int)tracks.size(); ++i) {
                            bool sel = (i == selectedTrack_);
                            if (ImGui::Selectable(tracks[i].c_str(), sel)) {
                                selectedTrack_ = i;
                                Play(cats[selectedCat_], tracks[i], true);
                            }
                            if (sel) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
            }
        }
    }

    float tempVolume = volume_;
    if (ImGui::SliderFloat("Volume", &tempVolume, 0.0f, 1.0f)) {
        SetVolume(tempVolume);
    }

    ImGui::End();

#endif // _DEBUG

}