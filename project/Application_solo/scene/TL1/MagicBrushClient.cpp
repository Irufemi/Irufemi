#include "MagicBrushClient.h"
#include "Engine/Graphics/DirectX/ShaderManager.h"
#include "../../../IrufemiEngine/Engine/Core/Utility/StringUtility.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <Windows.h>

MagicBrushClient::MagicBrushClient() : state_(State::Idle) {}

MagicBrushClient::~MagicBrushClient() {
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    StopPythonServer();
}

void MagicBrushClient::StartGeneration(const std::string& prompt, const std::string& referenceImagePath, const std::string& shaderName, const std::string& outputDirectory, ShaderManager* shaderManager) {
    if (state_ == State::Generating || state_ == State::Compiling || state_ == State::Fixing) {
        return; // 既に実行中
    }
    state_ = State::Generating;
    errorMessage_ = "";
    resultBlob_ = nullptr;

    if (workerThread_.joinable()) {
        workerThread_.join();
    }

    workerThread_ = std::thread(&MagicBrushClient::ProcessThread, this, prompt, referenceImagePath, shaderName, outputDirectory, shaderManager);
}

std::string MagicBrushClient::GetErrorMessage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return errorMessage_;
}

Microsoft::WRL::ComPtr<IDxcBlob> MagicBrushClient::GetResultBlob() {
    std::lock_guard<std::mutex> lock(mutex_);
    return resultBlob_;
}

bool MagicBrushClient::StartPythonServer() {
    if (IsServerRunning()) return true;

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // サーバーの黒窓を出さない
    ZeroMemory(&pi, sizeof(pi));

    // 実行ファイル(exe)のパスを取得し、そこから相対パスでToolsディレクトリを導き出す
    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::filesystem::path exePath(exePathBuf);
    
    // exeは WP0/generated/outputs/Editor/ にあるため、3階層上がって project/Tools/ShaderGenerator へ
    std::filesystem::path toolsDir = exePath.parent_path() / "../../../project/Tools/ShaderGenerator";
    std::string currentDir = std::filesystem::absolute(toolsDir).string();

    std::string command = "cmd.exe /c python main.py";
    std::vector<char> cmdBuffer(command.begin(), command.end());
    cmdBuffer.push_back('\0');

    if (!CreateProcessA(
        nullptr, 
        cmdBuffer.data(), 
        nullptr, 
        nullptr, 
        FALSE, 
        CREATE_NO_WINDOW, 
        nullptr, 
        currentDir.c_str(), 
        &si, 
        &pi)) {
        return false;
    }

    pythonProcessHandle_ = pi.hProcess;
    pythonThreadHandle_ = pi.hThread;
    pythonProcessId_ = pi.dwProcessId;
    
    // サーバーが立ち上がるのを少し待つ
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    return true;
}

void MagicBrushClient::StopPythonServer() {
    if (pythonProcessHandle_) {
        // cmd.exe経由で起動したため、子プロセスのpython.exeもまとめてツリー終了させる
        if (pythonProcessId_ != 0) {
            std::string killCmd = "taskkill /F /PID " + std::to_string(pythonProcessId_) + " /T";
            system(killCmd.c_str());
        }

        CloseHandle((HANDLE)pythonProcessHandle_);
        CloseHandle((HANDLE)pythonThreadHandle_);
        pythonProcessHandle_ = nullptr;
        pythonThreadHandle_ = nullptr;
        pythonProcessId_ = 0;
    }
}

void MagicBrushClient::RestartPythonServer() {
    StopPythonServer();
    StartPythonServer();
}

bool MagicBrushClient::IsServerRunning() const {
    if (!pythonProcessHandle_) return false;
    DWORD exitCode = 0;
    if (GetExitCodeProcess((HANDLE)pythonProcessHandle_, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return false;
}

std::string MagicBrushClient::EscapeJSON(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::string MagicBrushClient::SendPostRequest(const std::string& endpoint, const std::string& jsonPayload) {
    // テンポラリファイルにJSONを書き出す（エスケープ問題を避けるため）
    std::string tempFileName = "magic_brush_temp_req.json";
    std::ofstream ofs(tempFileName);
    if (!ofs) {
        return "ERROR: Failed to create temp request file.";
    }
    ofs << jsonPayload;
    ofs.close();

    // curl.exe をプロセスとして起動し、標準出力を取得する
    std::string command = "curl -s -X POST -H \"Content-Type: application/json\" -d @" + tempFileName + " http://127.0.0.1:8000" + endpoint;
    
    std::string result;
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        return "ERROR: _popen failed.";
    }
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    _pclose(pipe);
    
    return result;
}

void MagicBrushClient::ProcessThread(std::string prompt, std::string referenceImagePath, std::string shaderName, std::string outputDirectory, ShaderManager* shaderManager) {
    std::string currentHlsl = "";
    
    // 1. 初回リクエストの作成
    std::string jsonReq = "{ \"prompt\": \"" + EscapeJSON(prompt) + "\", \"image_path\": \"" + EscapeJSON(referenceImagePath) + "\" }";
    
    std::string hlslCode = SendPostRequest("/generate", jsonReq);
    
    if (hlslCode.find("ERROR:") == 0 || hlslCode.find("Internal Server Error") != std::string::npos) {
        std::lock_guard<std::mutex> lock(mutex_);
        errorMessage_ = "Failed to connect to Python Server or API Error.\n" + hlslCode;
        state_ = State::Error;
        return;
    }

    // 修復ループ
    int attempts = 0;
    while (attempts <= kMaxFixAttempts) {
        state_ = State::Compiling;
        
        // 取得したHLSLを一時ファイルに保存（コンパイル用）
        std::error_code ec;
        std::filesystem::create_directories(outputDirectory, ec); // ディレクトリが無ければ作成
        
        std::string filename = outputDirectory + shaderName + ".hlsl";
        // wstringへの変換 (Shift-JIS環境等でも動作する簡易変換)
        std::wstring tempHlslPath(filename.begin(), filename.end());
        
        {
            std::ofstream hlslFile(filename);
            hlslFile << hlslCode;
        }

        std::string errorLog;
        ShaderCompileOptions options;
        options.entryPoint = L"main";
        
        // コンパイル実行（キャッシュ破棄＋再コンパイル）
        auto blob = shaderManager->ReloadShader(tempHlslPath, options, L"ps_6_0", &errorLog);
        
        if (blob) {
            // コンパイル成功
            std::lock_guard<std::mutex> lock(mutex_);
            resultBlob_ = blob;
            state_ = State::Success;
            return;
        }
        
        // コンパイル失敗 -> 修復処理へ
        attempts++;
        if (attempts > kMaxFixAttempts) {
            std::lock_guard<std::mutex> lock(mutex_);
            errorMessage_ = "Max fix attempts reached.\nLast Error:\n" + errorLog;
            state_ = State::Error;
            return;
        }
        
        state_ = State::Fixing;
        
        // エラー修復リクエストの作成
        std::string fixReq = "{ \"error_log\": \"" + EscapeJSON(errorLog) + "\", \"code\": \"" + EscapeJSON(hlslCode) + "\" }";
        hlslCode = SendPostRequest("/fix_error", fixReq);
        
        if (hlslCode.find("ERROR:") == 0 || hlslCode.find("Internal Server Error") != std::string::npos) {
            std::lock_guard<std::mutex> lock(mutex_);
            errorMessage_ = "Failed to call /fix_error API.\n" + hlslCode;
            state_ = State::Error;
            return;
        }
    }
}
