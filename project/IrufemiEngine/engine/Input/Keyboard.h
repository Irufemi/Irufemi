#pragma once
#include <Windows.h>
#include <array>
#include <cstdint>

class Keyboard {
public:
    static const int KEY_COUNT = 256;

    Keyboard() = default;
    ~Keyboard() = default;

    void Initialize();
    void Update();

    // キー状態
    bool IsKeyDown(uint8_t key) const;
    bool IsKeyUp(uint8_t key) const;
    bool IsKeyPressed(uint8_t key) const;   // 立ち上がり
    bool IsKeyReleased(uint8_t key) const;  // 立ち下がり

    // DIK互換API
    bool IsKeyDownDIK(uint8_t dik) const;
    bool IsKeyUpDIK(uint8_t dik) const;
    bool IsKeyPressedDIK(uint8_t dik) const;
    bool IsKeyReleasedDIK(uint8_t dik) const;

private:
    std::array<BYTE, KEY_COUNT> currentKeys_{};
    std::array<BYTE, KEY_COUNT> previousKeys_{};

    // DIK -> VK 変換（表引き＋フォールバック）
    static uint8_t DIKToVK(uint8_t dik);
};
