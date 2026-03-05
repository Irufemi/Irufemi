#pragma once
#include <memory>
#include "engine/Input/Keyboard.h"
#include "engine/Input/GamePad.h"
#include "engine/Input/Mouse.h"
#include "math/Vector2.h"

// 役割：具体実装(Keyboard/GamePad/Mouse)を保持し、旧APIをフォワードして互換を維持するファサード
class InputManager {
public:
    InputManager() = default;
    ~InputManager() = default;

    void Initialize(HWND hwnd);
    void Update();

    // 新API(推奨)
    Keyboard* GetKeyboard() { return keyboard_.get(); }
    GamePad* GetGamePad() { return gamepad_.get(); }
    Mouse* GetMouse() { return mouse_.get(); }

    // 旧API(互換用：既存コードを壊さないためフォワード)
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

    // START ボタンフォワード
    bool StartDown() const;
    bool StartPressed() const;
    bool StartReleased() const;

    // --- D-Pad フォワード ---
    bool DPadUp() const;
    bool DPadDown() const;
    bool DPadLeft() const;
    bool DPadRight() const;
    bool DPadUpPressed() const;
    bool DPadDownPressed() const;
    bool DPadLeftPressed() const;
    bool DPadRightPressed() const;

    // --- マウスAPIフォワード ---
    bool IsMouseButtonDown(Mouse::Button button) const;
    bool IsMouseButtonPressed(Mouse::Button button) const;
    bool IsMouseButtonReleased(Mouse::Button button) const;
    const Vector2& GetMousePosition() const;
    const Vector2& GetMouseDelta() const;
    float GetMouseWheelDelta() const;


private:
    std::unique_ptr<Keyboard> keyboard_{};
    std::unique_ptr<GamePad>  gamepad_{};
    std::unique_ptr<Mouse>    mouse_{};
};