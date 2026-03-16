#include "Mouse.h"
#include <algorithm>
#include <string>

void Mouse::Initialize(HWND hwnd) {
    hwnd_ = hwnd;
    // 初期位置を取得
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd_, &p);
    position_ = { static_cast<float>(p.x), static_cast<float>(p.y) };
    prevPosition_ = position_;

    // 初期状態を適用
    SetLocked(isLocked_);
}

void Mouse::Update() {
    // 前フレームのボタン状態を保存
    std::copy(std::begin(currentButtons_), std::end(currentButtons_), std::begin(prevButtons_));

    // 現在のボタン状態を取得
    currentButtons_[0] = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
    currentButtons_[1] = (GetKeyState(VK_RBUTTON) & 0x8000) != 0;
    currentButtons_[2] = (GetKeyState(VK_MBUTTON) & 0x8000) != 0;

    // マウス位置の更新
    POINT p;
    GetCursorPos(&p);

    if (isLocked_) {
        // ロック中の挙動: スクリーン座標の p と中心との相対距離で Delta を出す
        RECT rc;
        GetClientRect(hwnd_, &rc);
        POINT center = { (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
        
        // 中心座標をスクリーン座標に変換
        POINT screenCenter = center;
        ClientToScreen(hwnd_, &screenCenter);

        // Delta を計算 (中心からの移動量)
        delta_ = { static_cast<float>(p.x - screenCenter.x), static_cast<float>(p.y - screenCenter.y) };

        // 常に中心に戻す (これにより、次のフレームの p がこの中心 + 移動量になる)
        SetCursorPos(screenCenter.x, screenCenter.y);

        // 内部の position_ はクライアント座標の中央に固定 (UI などのため)
        position_ = { static_cast<float>(center.x), static_cast<float>(center.y) };
        prevPosition_ = position_;
    } else {
        // 通常時の挙動: スクリーン座標をクライアント座標に変換して position_ を更新
        ScreenToClient(hwnd_, &p);
        prevPosition_ = position_;
        position_ = { static_cast<float>(p.x), static_cast<float>(p.y) };
        delta_ = { position_.x - prevPosition_.x, position_.y - prevPosition_.y };
    }

    // --- デバッグコード追加 ---
    if (wheelDelta_ != 0.0f) {
        std::string dbgMsg = "[Mouse::Update] wheelDelta_ before reset: " + std::to_string(wheelDelta_) + "\n";
        OutputDebugStringA(dbgMsg.c_str());
    }
    // -------------------------

    // ホイールの差分をリセット
    wheelDelta_ = 0.0f;
}

void Mouse::SetLocked(bool locked) {
    if (isLocked_ != locked) {
        isLocked_ = locked;
        delta_ = { 0.0f, 0.0f };

        if (isLocked_ && hwnd_) {
            // 表示を消す
            // ShowCursor はカウンタなので、ロック時は確実に非表示になるまで呼ぶ
            while (ShowCursor(FALSE) >= 0);

            // クリッピング設定
            RECT clientRect;
            GetClientRect(hwnd_, &clientRect);
            POINT topLeft = { clientRect.left, clientRect.top };
            POINT bottomRight = { clientRect.right, clientRect.bottom };
            ClientToScreen(hwnd_, &topLeft);
            ClientToScreen(hwnd_, &bottomRight);
            RECT clipRect = { topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
            ClipCursor(&clipRect);

            // 中央へ移動
            POINT center = { (clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2 };
            position_ = { static_cast<float>(center.x), static_cast<float>(center.y) };
            prevPosition_ = position_;

            ClientToScreen(hwnd_, &center);
            SetCursorPos(center.x, center.y);
        } else {
            // 表示を戻す
            while (ShowCursor(TRUE) < 0);
            
            // クリッピング解除
            ClipCursor(nullptr);
        }
    }
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