#include "Mouse.h"
#include <algorithm>

void Mouse::Initialize(HWND hwnd) {
    hwnd_ = hwnd;
    // 初期位置を取得
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd_, &p);
    position_ = { static_cast<float>(p.x), static_cast<float>(p.y) };
    prevPosition_ = position_;
}

void Mouse::Update() {
    // 前フレームのボタン状態を保存
    std::copy(std::begin(currentButtons_), std::end(currentButtons_), std::begin(prevButtons_));

    // 現在のボタン状態を取得
    currentButtons_[0] = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
    currentButtons_[1] = (GetKeyState(VK_RBUTTON) & 0x8000) != 0;
    currentButtons_[2] = (GetKeyState(VK_MBUTTON) & 0x8000) != 0;

    // マウス位置の更新
    prevPosition_ = position_;
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd_, &p);
    position_ = { static_cast<float>(p.x), static_cast<float>(p.y) };
    delta_ = { position_.x - prevPosition_.x, position_.y - prevPosition_.y };

    // ホイール値はイベント駆動で取得するため、ここではリセットしない
    // Win32Application側でWM_MOUSEWHEELを処理して値をセットすることを想定
}

bool Mouse::IsButtonDown(Button button) const {
    return currentButtons_[static_cast<int>(button)];
}

bool Mouse::IsButtonPressed(Button button) const {
    int index = static_cast<int>(button);
    return currentButtons_[index] && !prevButtons_[index];
}

bool Mouse::IsButtonReleased(Button button) const {
    int index = static_cast<int>(button);
    return !currentButtons_[index] && prevButtons_[index];
}

float Mouse::GetWheelDelta() const {
    // この実装では、外部からホイール値が設定されることを前提とします。
    // 例えば、Win32のメッセージループでWM_MOUSEWHEELを処理し、
    // このクラスのメンバー変数 wheelDelta_ を更新します。
    return wheelDelta_;
}