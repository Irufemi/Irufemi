#include "Core/System/DirectoryWatcher.h"
#include <windows.h>

DirectoryWatcher::DirectoryWatcher(const std::filesystem::path& targetDirectory, std::function<void()> onChangeCallback)
    : targetDirectory_(targetDirectory), onChangeCallback_(onChangeCallback), isRunning_(true) {

    // ディレクトリハンドルの取得
    directoryHandle_ = CreateFileW(targetDirectory_.c_str(), FILE_LIST_DIRECTORY,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                                   FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (directoryHandle_ == INVALID_HANDLE_VALUE) {
        directoryHandle_ = nullptr;
        isRunning_ = false;
        return;
    }

    workerThread_ = std::thread(&DirectoryWatcher::WatchLoop, this);
}

DirectoryWatcher::~DirectoryWatcher() {
    isRunning_ = false;
    if (directoryHandle_) {
        // CancelIoEx を用いて非同期の待機を強制キャンセルする（Windows Vista以降）
        CancelIoEx(directoryHandle_, NULL);
        CloseHandle(directoryHandle_);
        directoryHandle_ = nullptr;
    }
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void DirectoryWatcher::WatchLoop() {
    alignas(DWORD) char buffer[4096];
    DWORD bytesReturned = 0;

    while (isRunning_) {
        BOOL result = ReadDirectoryChangesW(directoryHandle_, buffer, sizeof(buffer),
                                            TRUE, // サブディレクトリも監視
                                            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                                FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                                                FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
                                            &bytesReturned, NULL, NULL);

        if (!result || bytesReturned == 0) {
            // ハンドルが閉じられたかエラーが発生した場合はループを抜ける
            break;
        }

        // --- デバウンス処理 (Debounce) ---
        // エディタ保存やビルド時に複数のファイル変更通知が連続して飛ぶため、
        // 変更が落ち着くまで一定時間待機して1回にまとめる
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        if (isRunning_ && onChangeCallback_) {
            onChangeCallback_();
        }
    }
}
