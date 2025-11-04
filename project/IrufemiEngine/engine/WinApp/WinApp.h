#pragma once

#include <Windows.h>
#include <string>
#include <cstdint>

class WinApp final {
public:
    // クラス定数（課題要件の int 定数）
    static constexpr int kClassVersion = 1;

    WinApp() = default;
    ~WinApp();

    // 初期化（ウィンドウ生成 + COM 初期化）
    bool Initialize(HINSTANCE hInstance, int width = 1280, int height = 720, const std::wstring& title = L"Window");

    // 後始末（ウィンドウ破棄 + COM 終了）
    void Finalize();

    // メッセージポンプ（false を返したらアプリ終了）
    bool ProcessMessages();

    // ゲッター
    HWND GetHwnd() const { return hwnd_; }
    HINSTANCE GetHInstance() const { return hInstance_; }
    int GetClientWidth() const { return clientWidth_; }
    int GetClientHeight() const { return clientHeight_; }

    // 静的 WndProc
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // コピー/ムーブ禁止
    WinApp(const WinApp&) = delete;
    WinApp& operator=(const WinApp&) = delete;
    WinApp(WinApp&&) = delete;
    WinApp& operator=(WinApp&&) = delete;

private:
    // 非静的メッセージ処理本体
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HINSTANCE hInstance_ = nullptr;
    HWND hwnd_ = nullptr;
    std::wstring className_ = L"IrufemiWinClass";
    std::wstring windowTitle_;
    int32_t clientWidth_ = 0;
    int32_t clientHeight_ = 0;
    bool comInitialized_ = false;
    bool didRegisterClass_ = false;
};
