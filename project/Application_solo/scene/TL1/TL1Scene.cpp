#include "TL1Scene.h"
#include "MagicBrushClient.h"
#include "Engine/Irufemi.h"
#include "Engine/Graphics/DirectX/ShaderCompiler.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"
#include "Engine/Graphics/DirectX/RootSignatureConfig.h"
#include "Resource/Texture/TextureManager.h"

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
        
        // 入力された画像パスがあればそれをテクスチャとしてロードしてセット、無ければダミー(白)をセットする
        D3D12_GPU_DESCRIPTOR_HANDLE texHandle = engine_->GetTextureManager()->GetWhiteTextureHandle();
        if (!imagePath_.empty()) {
            texHandle = engine_->GetTextureManager()->GetTextureHandle(imagePath_);
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
        static char imagePathBuffer[256] = "";

        // サーバー状態と再起動UI
        ImGui::Separator();
        bool isRunning = magicBrushClient_ && magicBrushClient_->IsServerRunning();
        ImGui::Text("Server Status: %s", isRunning ? "Running" : "Stopped");
        if (isRunning) {
            if (ImGui::Button("Restart Server")) {
                magicBrushClient_->RestartPythonServer();
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop Server")) {
                magicBrushClient_->StopPythonServer();
            }
        } else {
            if (ImGui::Button("Start Server")) {
                magicBrushClient_->StartPythonServer();
            }
        }
        ImGui::Separator();

        ImGui::InputText("Prompt", promptBuffer, sizeof(promptBuffer));
        ImGui::InputText("Image Path", imagePathBuffer, sizeof(imagePathBuffer));

        // サーバーが動いていない時はボタンを無効化する
        if (!isRunning) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Generate Shader")) {
            promptText_ = promptBuffer;
            imagePath_ = imagePathBuffer;

            if (magicBrushClient_) {
                isShaderRegistered_ = false; // 再登録できるようにフラグをリセット
                magicBrushClient_->StartGeneration(promptText_, imagePath_);
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

        ImGui::End();
    }
#endif
}
