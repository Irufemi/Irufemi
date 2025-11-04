#include "InputManager.h"

void InputManager::Initialize() {
    keyboard_ = std::make_unique<Keyboard>();
    gamepad_ = std::make_unique<GamePad>();
    keyboard_->Initialize();
    gamepad_->Initialize();
}

void InputManager::Update() {
    keyboard_->Update();
    gamepad_->Update();
}

// --- 旧APIフォワード（互換維持）---
bool InputManager::IsKeyDown(uint8_t k)     const { return keyboard_->IsKeyDown(k); }
bool InputManager::IsKeyUp(uint8_t k)       const { return keyboard_->IsKeyUp(k); }
bool InputManager::IsKeyPressed(uint8_t k)  const { return keyboard_->IsKeyPressed(k); }
bool InputManager::IsKeyReleased(uint8_t k) const { return keyboard_->IsKeyReleased(k); }

bool InputManager::IsButtonDown(WORD b)     const { return gamepad_->IsButtonDown(b); }
bool InputManager::IsButtonUp(WORD b)       const { return gamepad_->IsButtonUp(b); }
bool InputManager::IsButtonPressed(WORD b)  const { return gamepad_->IsButtonPressed(b); }
bool InputManager::IsButtonReleased(WORD b) const { return gamepad_->IsButtonReleased(b); }

float InputManager::GetLeftStickX()  const { return gamepad_->GetLeftStickX(); }
float InputManager::GetLeftStickY()  const { return gamepad_->GetLeftStickY(); }
float InputManager::GetRightStickX() const { return gamepad_->GetRightStickX(); }
float InputManager::GetRightStickY() const { return gamepad_->GetRightStickY(); }
float InputManager::GetLeftTrigger() const { return gamepad_->GetLeftTrigger(); }
float InputManager::GetRightTrigger()const { return gamepad_->GetRightTrigger(); }

bool InputManager::IsKeyDownDIK(uint8_t d)     const { return keyboard_->IsKeyDownDIK(d); }
bool InputManager::IsKeyUpDIK(uint8_t d)       const { return keyboard_->IsKeyUpDIK(d); }
bool InputManager::IsKeyPressedDIK(uint8_t d)  const { return keyboard_->IsKeyPressedDIK(d); }
bool InputManager::IsKeyReleasedDIK(uint8_t d) const { return keyboard_->IsKeyReleasedDIK(d); }