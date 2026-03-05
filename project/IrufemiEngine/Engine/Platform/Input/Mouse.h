#pragma once
#include <Windows.h>
#include "math/Vector2.h"

class Mouse {
public:
    enum class Button {
        Left,
        Right,
        Middle,
    };

    Mouse() = default;
    ~Mouse() = default;

    void Initialize(HWND hwnd);
    void Update();

    // ボタン入力
    bool IsButtonDown(Button button) const;
    bool IsButtonPressed(Button button) const;
    bool IsButtonReleased(Button button) const;

    // マウス位置
    const Vector2& GetPosition() const { return position_; }
    const Vector2& GetDelta() const { return delta_; }

    // ホイール
    float GetWheelDelta() const;

    // WinAppからホイール差分を設定するためのセッター
    void SetWheelDelta(float delta) { wheelDelta_ = delta; }

private:
    HWND hwnd_ = nullptr;
    BYTE currentButtons_[3]{};
    BYTE prevButtons_[3]{};
    Vector2 position_{};
    Vector2 prevPosition_{};
    Vector2 delta_{};
    float wheelDelta_ = 0.0f;
};