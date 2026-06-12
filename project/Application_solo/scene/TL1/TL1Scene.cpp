#include "TL1Scene.h"
#include "Engine/Irufemi.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

/**
 * @brief 初期化
 */
void TL1Scene::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
    // TODO: カメラや専用の描画パイプラインなどの初期化
}

/**
 * @brief 更新
 */
void TL1Scene::Update() {
    // 毎フレームのロジック更新
}

/**
 * @brief 描画
 */
void TL1Scene::Draw() {
    // TODO: 生成したシェーダーエフェクトの描画
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

        ImGui::InputText("Prompt", promptBuffer, IM_ARRAYSIZE(promptBuffer));
        ImGui::InputText("Image Path", imagePathBuffer, IM_ARRAYSIZE(imagePathBuffer));

        if (ImGui::Button("Generate Shader")) {
            promptText_ = promptBuffer;
            imagePath_ = imagePathBuffer;

            // TODO: PythonのFastAPIサーバーへ非同期HTTPリクエストを送信する処理
        }

        ImGui::End();
    }
#endif
}
