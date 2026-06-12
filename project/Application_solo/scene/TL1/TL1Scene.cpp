#include "TL1Scene.h"
#include "MagicBrushClient.h"
#include "Engine/Irufemi.h"
#include "Engine/Graphics/DirectX/ShaderCompiler.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"
#include "Engine/Graphics/DirectX/RootSignatureConfig.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/Platform/WindowsAPI/WinApp.h"
#include <commdlg.h>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

TL1Scene::~TL1Scene() = default;

/**
 * @brief 初期化
 */
void TL1Scene::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
    magicBrushClient_ = std::make_unique<MagicBrushClient>();
    
    // フルスクリーン描画用の頂点シェーダーをコンパイル
    ShaderCompiler compiler;
    compiler.Initialize();
    std::string errorLog;
    ShaderCompileOptions options;
    options.entryPoint = L"main";
    vsBlob_ = compiler.Compile(L"resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", options, &errorLog);

    // Pythonサーバーの自動起動
    magicBrushClient_->StartPythonServer();
}

/**
 * @brief 更新
 */
void TL1Scene::Update() {
    // 成功したシェーダーをPSOに登録する
    if (magicBrushClient_ && !isShaderRegistered_ && vsBlob_) {
        if (magicBrushClient_->GetState() == MagicBrushClient::State::Success) {
            auto psBlob = magicBrushClient_->GetResultBlob();
            if (psBlob) {
                engine_->GetPSOManager()->RegisterShader("MagicBrushPS", { { vsBlob_, psBlob, nullptr } });
                isShaderRegistered_ = true;
            }
        }
    }
}

/**
 * @brief 描画
 */
void TL1Scene::Draw() {
    // プレビュー描画
    if (isShaderRegistered_) {
        engine_->ApplyPSO("MagicBrushPS");
        auto cmd = engine_->GetCommandList();
        
        // 入力されたテクスチャパス(名)があればそれをテクスチャとしてロードしてセット、無ければダミー(白)をセットする
        D3D12_GPU_DESCRIPTOR_HANDLE texHandle = engine_->GetTextureManager()->GetWhiteTextureHandle();
        if (!textureImagePath_.empty()) {
            texHandle = engine_->GetTextureManager()->GetTextureHandle(textureImagePath_);
        }
        cmd->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootSlot::Texture), texHandle);

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->DrawInstanced(3, 1, 0, 0);
    }
}

/**
 * @brief UIの描画（デバッグタブ）
 */
void TL1Scene::DrawDebugTab() {
#ifdef USE_IMGUI
    if (ImGui::Begin("AI Magic Brush")) {
        ImGui::Text("AI Shader Generator Interface");
        ImGui::Separator();

        // 文字列入力用のバッファ（静的変数にして入力を保持）
        static char promptBuffer[512] = "";
        static char refImagePathBuffer[MAX_PATH] = "";
        
        // 簡易通知メッセージ用の静的変数
        static std::string notificationMsg = "";
        static float notificationTimer = 0.0f;
        
        // D&Dの受け取り
        if (engine_ && engine_->GetWinApp()) {
            std::string dropped = engine_->GetWinApp()->GetDroppedFilePath();
            if (!dropped.empty()) {
                strncpy_s(refImagePathBuffer, dropped.c_str(), _TRUNCATE);
                engine_->GetWinApp()->ClearDroppedFilePath();
            }
        }

        // サーバー状態と再起動UI
        ImGui::Separator();
        bool isRunning = magicBrushClient_ && magicBrushClient_->IsServerRunning();
        ImGui::Text("Server Status: %s", isRunning ? "Running" : "Stopped");
        if (isRunning) {
            if (ImGui::Button("Restart Server")) {
                magicBrushClient_->RestartPythonServer();
                notificationMsg = "Python Server Restarted.";
                notificationTimer = 3.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop Server")) {
                magicBrushClient_->StopPythonServer();
                notificationMsg = "Python Server Stopped.";
                notificationTimer = 3.0f;
            }
        } else {
            if (ImGui::Button("Start Server")) {
                magicBrushClient_->StartPythonServer();
                notificationMsg = "Python Server Started.";
                notificationTimer = 3.0f;
            }
        }
        ImGui::Separator();

        ImGui::InputText("Prompt", promptBuffer, sizeof(promptBuffer));
        
        // Reference Image (AI参考画像)
        ImGui::InputText("Reference Image", refImagePathBuffer, sizeof(refImagePathBuffer));
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            OPENFILENAMEA ofn;
            char szFile[MAX_PATH] = { 0 };
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = engine_->GetWinApp() ? engine_->GetWinApp()->GetHwnd() : nullptr;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg\0All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            if (GetOpenFileNameA(&ofn) == TRUE) {
                strncpy_s(refImagePathBuffer, ofn.lpstrFile, _TRUNCATE);
            }
        }
        
        // Input Texture (C++バインド用)
        static int selectedTextureIndex = 0;
        std::vector<std::string> textureNames = engine_->GetTextureManager()->GetTextureNames();
        // コンボボックス用に const char* の配列を作る
        std::vector<const char*> comboItems;
        comboItems.push_back("None (White Texture)"); // 先頭に空要素を入れる
        for (const auto& name : textureNames) {
            comboItems.push_back(name.c_str());
        }
        if (ImGui::Combo("Input Texture", &selectedTextureIndex, comboItems.data(), static_cast<int>(comboItems.size()))) {
            if (selectedTextureIndex == 0) {
                textureImagePath_ = "";
            } else {
                textureImagePath_ = textureNames[selectedTextureIndex - 1];
            }
        }

        // サーバーが動いていない時はボタンを無効化する
        if (!isRunning) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Generate Shader")) {
            promptText_ = promptBuffer;
            referenceImagePath_ = refImagePathBuffer;

            if (magicBrushClient_) {
                isShaderRegistered_ = false; // 再登録できるようにフラグをリセット
                magicBrushClient_->StartGeneration(promptText_, referenceImagePath_);
            }
        }
        if (!isRunning) {
            ImGui::EndDisabled();
        }

        if (magicBrushClient_) {
            ImGui::Separator();
            auto state = magicBrushClient_->GetState();
            switch (state) {
                case MagicBrushClient::State::Idle: ImGui::Text("Status: Idle"); break;
                case MagicBrushClient::State::Generating: ImGui::TextColored(ImVec4(1, 1, 0, 1), "Status: Generating initial HLSL..."); break;
                case MagicBrushClient::State::Compiling: ImGui::TextColored(ImVec4(1, 1, 0, 1), "Status: Compiling..."); break;
                case MagicBrushClient::State::Fixing: ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Status: Fixing Compile Errors..."); break;
                case MagicBrushClient::State::Success: ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Success!"); break;
                case MagicBrushClient::State::Error: 
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: Error");
                    ImGui::TextWrapped("%s", magicBrushClient_->GetErrorMessage().c_str());
                    break;
            }
        }

        // 通知メッセージの描画とタイマー減算
        if (notificationTimer > 0.0f) {
            notificationTimer -= ImGui::GetIO().DeltaTime;
            // フェードアウト効果 (アルファ値をタイマーに連動させる)
            float alpha = notificationTimer > 1.0f ? 1.0f : notificationTimer;
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, alpha), "[System] %s", notificationMsg.c_str());
        }

        ImGui::End();
    }
#endif
}
