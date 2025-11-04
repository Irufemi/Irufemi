#pragma once

#include <cstdint>

// 前方宣言
class IrufemiEngine;

/// <summary>
/// Scene系クラスに継承する基底クラス
/// </summary>
class IScene {
public:
    virtual ~IScene() = default;
    virtual void Initialize(IrufemiEngine* engine) = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;

    // 毎フレーム、SceneManager から呼ぶ
    static void SyncInput(IrufemiEngine* engine);

    // ── 簡易ヘルパ（VK版） ──
    static inline bool DownVK(uint8_t vk) { return (keys_[vk] & 0x80) != 0; }
    static inline bool PressedVK(uint8_t vk) { return !(preKeys_[vk] & 0x80) && (keys_[vk] & 0x80); }
    static inline bool ReleasedVK(uint8_t vk) { return  (preKeys_[vk] & 0x80) && !(keys_[vk] & 0x80); }

    // ── 簡易ヘルパ（DIK版） ──
    static inline bool DownDIK(uint8_t dik) { return (dikKeys_[dik] & 0x80) != 0; }
    static inline bool PressedDIK(uint8_t dik) { return !(dikPreKeys_[dik] & 0x80) && (dikKeys_[dik] & 0x80); }
    static inline bool ReleasedDIK(uint8_t dik) { return  (dikPreKeys_[dik] & 0x80) && !(dikKeys_[dik] & 0x80); }

private:
    static unsigned char keys_[256];
    static unsigned char preKeys_[256];
    static unsigned char dikKeys_[256];
    static unsigned char dikPreKeys_[256];
};