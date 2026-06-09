#pragma once
#include <xaudio2.h>
#include <atomic>

// SourceVoiceの再生が完了したことを受け取り、自身を破棄するためのコールバック
class VoiceCallback : public IXAudio2VoiceCallback {
public:
    virtual ~VoiceCallback() {}
    void OnStreamEnd() override {}
    void OnVoiceProcessingPassEnd() override {}
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) override {}
    void OnBufferEnd(void* pBufferContext) override { 
        finished_ = true; 
    }
    void OnBufferStart(void* pBufferContext) override {}
    void OnLoopEnd(void* pBufferContext) override {}
    void OnVoiceError(void* pBufferContext, HRESULT Error) override {}

    bool IsFinished() const { return finished_; }

private:
    std::atomic<bool> finished_{ false };
};