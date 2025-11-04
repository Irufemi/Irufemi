#include "WinApp.h"

#include "../../externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma comment(lib,"winmm.lib")

WinApp::~WinApp() {
    Finalize();
}

bool WinApp::Initialize(HINSTANCE hInstance, int width, int height, const std::wstring& title) {
    
    hInstance_ = hInstance;
    clientWidth_ = width;
    clientHeight_ = height;
    windowTitle_ = title;

    // システムタイマーの分解能を上げる
    timeBeginPeriod(1);


    // ─────────────────────────────────────────────────────
    // 学校資料準拠：WNDCLASS + RegisterClass + CreateWindow
    // ─────────────────────────────────────────────────────

    /*ウィンドウを作ろう*/

    ///ウィンドウクラスを登録する

    WNDCLASSW wc{};
    //ウィンドウプロシージャ
    wc.lpfnWndProc = &WinApp::WndProc;
    //ウィンドウクラス名(なんでもいい)
    wc.lpszClassName = className_.c_str();
    //インスタンスハンドル
    wc.hInstance = hInstance;
    //カーソル
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    //ウィンドウクラスを登録する
    ATOM atom = RegisterClassW(&wc);
    didRegisterClass_ = (atom != 0);           // 既に登録済みなら 0（解除しない）

    ///ウィンドウサイズを決める

    //ウィンドウサイズを表す構造体にクライアント領域を入れる
    RECT wrc = { 0,0,clientWidth_ ,clientHeight_ };

    //クライアント領域をもとに実際のサイズにwrcを変更してもらう
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    ///ウィンドウを生成して表示

    //ウィンドウの生成
    hwnd_ = CreateWindowW(
        wc.lpszClassName,		//利用するクラス名
        windowTitle_.c_str(),			        //タイトルバーの文字(何でも良い)
        WS_OVERLAPPEDWINDOW,	//よく見るウィンドウスタイル
        CW_USEDEFAULT,			//表示X座標(windowsに任せる)
        CW_USEDEFAULT,			//表示Y座標(windowsに任せる)
        wrc.right - wrc.left,	//ウィンドウ横幅
        wrc.bottom - wrc.top,	//ウィンドウ縦幅
        nullptr,				//親ウィンドウハンドル
        nullptr,				//メニューハンドル
        wc.hInstance,			//インスタンスハンドル
        this					//オプション
    );
    if (!hwnd_) {
        if (comInitialized_) { CoUninitialize(); comInitialized_ = false; }
        return false;
    }

    /*ウィンドウを作ろう*/

    ///ウィンドウを生成して表示

    //ウィンドウを表示する
    ShowWindow(hwnd_, SW_SHOW);
    SetWindowTextW(hwnd_, windowTitle_.c_str());

    // 実クライアントサイズ
    RECT cr{};
    GetClientRect(hwnd_, &cr);
    clientWidth_ = cr.right - cr.left;
    clientHeight_ = cr.bottom - cr.top;

    // ─────────────────────────────────────────────────────
    // 推奨（WinApp集約の modern 版：後で差し替えるときの参考）
    //  - WNDCLASSEXW/ RegisterClassExW
    //  - static WinApp::WndProc + WM_NCCREATE で this 紐付け
    //  - CreateWindowExW で lpParam に this を渡す
    //  - 背景フラッシュ回避のため背景ブラシは nullptr 推奨
    // ─────────────────────────────────────────────────────
    //WNDCLASSEXW wcx{};
    //wcx.cbSize = sizeof(wcx);
    //wcx.style = CS_HREDRAW | CS_VREDRAW;
    //wcx.lpfnWndProc = &WinApp::WndProc;
    //wcx.hInstance = hInstance_;
    //wcx.hCursor = LoadCursor(nullptr, IDC_ARROW);
    //wcx.hbrBackground = nullptr; // ちらつき回避
    //wcx.lpszClassName = className_.c_str();
    //if (!RegisterClassExW(&wcx)) {
    //    if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
    //        if (comInitialized_) { CoUninitialize(); comInitialized_ = false; }
    //        return false;
    //    }
    //    didRegisterClass_ = false;
    //} else {
    //    didRegisterClass_ = true;
    //}

    //RECT rc{ 0, 0, width, height };
    //AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    //const int winW = rc.right - rc.left;
    //const int winH = rc.bottom - rc.top;

    //hwnd_ = CreateWindowExW(
    //    0,
    //    className_.c_str(),
    //    windowTitle_.c_str(),
    //    WS_OVERLAPPEDWINDOW,
    //    CW_USEDEFAULT, CW_USEDEFAULT,
    //    winW, winH,
    //    nullptr, nullptr, hInstance_, this // ← lpParam に this
    //);
    //if (!hwnd_) {
    //    if (comInitialized_) { CoUninitialize(); comInitialized_ = false; }
    //    return false;
    //}

    //ShowWindow(hwnd_, SW_SHOW);
    //// UpdateWindow(hwnd_); // 連続描画なら不要
    //RECT cr2{};
    //GetClientRect(hwnd_, &cr2);
    //clientWidth_ = cr2.right - cr2.left;
    //clientHeight_ = cr2.bottom - cr2.top;

    return true;
}

void WinApp::Finalize() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    if (didRegisterClass_ && !className_.empty() && hInstance_) { // 自分が登録した場合のみ解除
        UnregisterClassW(className_.c_str(), hInstance_);
        didRegisterClass_ = false;
    }
    if (comInitialized_) {
        CoUninitialize();
        comInitialized_ = false;
    }
}

bool WinApp::ProcessMessages() {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

LRESULT CALLBACK WinApp::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* pThis = reinterpret_cast<WinApp*>(cs->lpCreateParams);
        if (pThis) {
            pThis->hwnd_ = hWnd; // ここで先に設定
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        }
        return TRUE; // NCCREATE 成功
    }

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return TRUE;
    }

    if (auto* pThis = reinterpret_cast<WinApp*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA))) {
        return pThis->HandleMessage(hWnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT WinApp::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        clientWidth_ = LOWORD(lParam);
        clientHeight_ = HIWORD(lParam);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}