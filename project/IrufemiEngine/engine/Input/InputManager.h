#pragma once
#include <memory>
#include "engine/Input/Keyboard.h"
#include "engine/Input/GamePad.h"

// 役割：具体実装(Keyboard/GamePad)を保持し、旧APIをフォワードして互換を維持するファサード
class InputManager {
public:
    InputManager() = default;
    ~InputManager() = default;

    void Initialize();
    void Update();

    // 新API（推奨）
    Keyboard* GetKeyboard() { return keyboard_.get(); }
    GamePad* GetGamePad() { return gamepad_.get(); }

    // 旧API（互換用：既存コードを壊さないためフォワード）
    bool IsKeyDown(uint8_t key)     const;
    bool IsKeyUp(uint8_t key)       const;
    bool IsKeyPressed(uint8_t key)  const;
    bool IsKeyReleased(uint8_t key) const;

    bool IsButtonDown(WORD button)     const;
    bool IsButtonUp(WORD button)       const;
    bool IsButtonPressed(WORD button)  const;
    bool IsButtonReleased(WORD button) const;

    float GetLeftStickX()  const;
    float GetLeftStickY()  const;
    float GetRightStickX() const;
    float GetRightStickY() const;

    float GetLeftTrigger()  const;
    float GetRightTrigger() const;

    bool IsKeyDownDIK(uint8_t dik) const;
    bool IsKeyUpDIK(uint8_t dik) const;
    bool IsKeyPressedDIK(uint8_t dik) const;
    bool IsKeyReleasedDIK(uint8_t dik) const;

private:
    std::unique_ptr<Keyboard> keyboard_{};
    std::unique_ptr<GamePad>  gamepad_{};
};