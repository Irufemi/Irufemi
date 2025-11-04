#define NOMINMAX
#include "DebugUI.h"

#include <Windows.h>

/*開発のUIを出そう*/

#include "../externals/imgui/imgui.h"
#include "../externals/imgui/imgui_impl_dx12.h"
#include "../externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ImGuiWindowFlags_NoDocking が未定義の場合は定義する
#ifndef ImGuiWindowFlags_NoDocking
#define ImGuiWindowFlags_NoDocking (1 << 13)
#endif

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <numeric> 
#include "../manager/TextureManager.h"

void DebugUI::Initialize(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList, const Microsoft::WRL::ComPtr<ID3D12Device>& device, HWND& hwnd, DXGI_SWAP_CHAIN_DESC1& swapChainDesc, D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, ID3D12DescriptorHeap* srvDescriptorHeap) {

    this->commandList_ = commandList;

    /*開発UIを出そう*/
    //ImGuiの初期化。詳細はさして重要ではないので開設は省略する。
    //こういうもんである
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(
        device.Get(),
        swapChainDesc.BufferCount,
        rtvDesc.Format,
        srvDescriptorHeap,
        srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
    );

}

void DebugUI::FrameStart() {

    /*開発のUIを出そう*/

    ///ImGuiを使う
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

}

void DebugUI::Shutdown() {

    if (commandList_) { commandList_.Reset(); }
    /*開発のUIを出そう*/

    ///ImGuiの終了処理

    //ImGuiの終了処理。詳細はさして重要ではないので解説は省略する。
    //こういうもんである。初期化と逆順に行う。
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

}

void DebugUI::QueueDrawCommands() {

    /*開発のUIを出そう*/

    ///ImGuiを使う

    //ImGuiの内部コマンドを生成する
    ImGui::Render();
}

void DebugUI::QueuePostDrawCommands() {

    /*開発のUIを出そう*/

    ///ImGuiを描画する

    //実際のcommandListのImGuiの描画コマンドを積む
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_.Get());

}

// transform
void DebugUI::DebugTransform(Transform& transform) {
    if (ImGui::CollapsingHeader("transform")) {
        ImGui::DragFloat3("scale", &transform.scale.x, 0.05f);
        ImGui::DragFloat3("rotate", &transform.rotate.x, 0.05f);
        ImGui::DragFloat3("translate", &transform.translate.x, 0.05f);
        static bool rotateX = false;
        ImGui::Checkbox("RotateX", &rotateX);
        if (rotateX) {
            transform.rotate.x += static_cast<float>(0.05f / std::numbers::pi);
        }
        static bool rotateY = false;
        ImGui::Checkbox("RotateY", &rotateY);
        if (rotateY) {
            transform.rotate.y += static_cast<float>(0.05f / std::numbers::pi);
        }
        static bool rotateZ = false;
        ImGui::Checkbox("RotateZ", &rotateZ);
        if (rotateZ) {
            transform.rotate.z += static_cast<float>(0.05f / std::numbers::pi);
        }
    }
}

// transform
void DebugUI::DebugTransform2D(Transform& transform) {
    if (ImGui::CollapsingHeader("transform")) {
        ImGui::DragFloat2("scale", &transform.scale.x, 0.05f);
        ImGui::DragFloat("rotate", &transform.rotate.z, 0.05f);
        ImGui::DragFloat2("translate", &transform.translate.x, 0.05f);
        static bool rotate = false;
        ImGui::Checkbox("Rotate", &rotate);
        if (rotate) {
            transform.rotate.z += static_cast<float>(0.05f / std::numbers::pi);
        }
    }
}

void DebugUI::TextTransform(Transform& transform,const char* name) {
    std::string header = std::string("transform") + name;
    if (ImGui::CollapsingHeader(header.c_str())) {
        ImGui::Text("scale: (%.2f, %.2f, %.2f)", transform.scale.x, transform.scale.y, transform.scale.z);
        ImGui::Text("rotate: (%.2f, %.2f, %.2f)", transform.rotate.x, transform.rotate.y, transform.rotate.z);
        ImGui::Text("translate: (%.2f, %.2f, %.2f)", transform.translate.x, transform.translate.y, transform.translate.z);
    }
}


// Material
void DebugUI::DebugMaterialBy3D(Material* materialData) {
    if (ImGui::CollapsingHeader("material")) {
        ImGui::ColorEdit4("spriteColor", &materialData->color.x);
        bool enableLighting = materialData->enableLighting;
        if (ImGui::Checkbox("enableLighting", &enableLighting)) {
            materialData->enableLighting = enableLighting;
        }
        // lightingMode選択
        const char* items[] = { "NonLighting", "Lambert", "HalfLambert" };
        int currentMode = materialData->lightingMode;
        if (ImGui::Combo("LightingMode", &currentMode, items, IM_ARRAYSIZE(items))) {
            materialData->lightingMode = currentMode;
        }
        ImGui::DragFloat("Shininess", &materialData->shininess);
    }
}

// Material
void DebugUI::DebugMaterialBy2D(Material* materialData) {
    if (ImGui::CollapsingHeader("material")) {
        ImGui::ColorEdit4("spriteColor", &materialData->color.x);
    }
}

// 画像
void DebugUI::DebugTexture(D3D12ResourceUtil* resource, int& selectedTextureIndex) {

    if (ImGui::CollapsingHeader("texture")) {

        std::vector<std::string> textureNames = textureManager_->GetTextureNames();
        std::sort(textureNames.begin(), textureNames.end());

        if (!textureNames.empty()) {
            selectedTextureIndex = std::clamp(selectedTextureIndex, 0, static_cast<int>(textureNames.size()) - 1);
            if (ImGui::BeginCombo("TextureName", textureNames[selectedTextureIndex].c_str())) {
                for (int i = 0; i < textureNames.size(); ++i) {
                    bool isSelected = (i == selectedTextureIndex);
                    if (ImGui::Selectable(textureNames[i].c_str(), isSelected)) {
                        selectedTextureIndex = i;
                        resource->textureHandle_ = textureManager_->GetTextureHandle(textureNames[i]);
                    }
                }
                ImGui::EndCombo();
            }
        } else {
            ImGui::Text("No textures found.");
        }
    }
}

// DirectionalLight
void DebugUI::DebugDirectionalLight(DirectionalLight* directionalLightData) {
    if (ImGui::CollapsingHeader("directionalLight")) {
        ImGui::ColorEdit4("lightColor", &directionalLightData->color.x);
        ImGui::DragFloat3("lightDirection", &directionalLightData->direction.x, 0.01f);
        ImGui::DragFloat("intensity", &directionalLightData->intensity, 0.01f, 0.0f);
    }
}

// UvTransform
void DebugUI::DebugUvTransform(Transform& uvTransform) {
    if (ImGui::CollapsingHeader("uvTransform")) {
        ImGui::DragFloat3("UVTranslate", &uvTransform.translate.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat3("UVScale", &uvTransform.scale.x, 0.01f, -10.0f, 10.0f);
        ImGui::SliderAngle("UVRotate", &uvTransform.rotate.z);
    }
}

// Sphere
void DebugUI::DebugSphereInfo(Sphere& sphere) {
    if (ImGui::CollapsingHeader("info")) {
        ImGui::DragFloat3("Center", &sphere.center.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat("radius", &sphere.radius, 0.01f, -10.0f, 10.0f);
    }
}

// FPS/FrameTime オーバーレイ
void DebugUI::FPSDebug() {
    ImGuiIO& io = ImGui::GetIO();
    const float fpsNow = io.Framerate;
    const float frameMsNow = (fpsNow > 0.0f) ? (1000.0f * io.DeltaTime) : 0.0f;

    UpdatePerfStats_(frameMsNow);
    cachedFps_ = fpsNow;

    // ウィンドウ
    ImGui::SetNextWindowBgAlpha(0.50f);
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("Performance", nullptr, flags)) {
        const size_t count = historyFilled_ ? kPerfHistoryCount_ : historyIndex_;
        const float maxScale = std::max(20.0f, cachedMaxMs_ * 1.15f);
        const float target60 = 1000.0f / 60.0f;
        const float target30 = 1000.0f / 30.0f;

        ImGui::Text("FPS:  %.1f", cachedFps_);
        ImGui::Text("Now:  %.2f ms", frameMsNow);
        ImGui::Separator();
        ImGui::Text("Avg:  %.2f ms (%.1f FPS)", cachedAvgMs_, 1000.0f / std::max(0.0001f, cachedAvgMs_));
        ImGui::Text("Min:  %.2f ms", cachedMinMs_);
        ImGui::Text("Max:  %.2f ms", cachedMaxMs_);
        ImGui::Text("P99:  %.2f ms (1%% low ~%.1f FPS)", cachedP99Ms_, 1000.0f / std::max(0.0001f, cachedP99Ms_));
        ImGui::Text("Frame Count: %zu", count);

        ImGui::Dummy(ImVec2(0, 4));

        /*
        // ラインプロット
        ImGui::PlotLines("Frame (ms)", frameTimeHistory_.data(),
            static_cast<int>(count),
            0,
            nullptr,
            0.0f,
            maxScale,
            ImVec2(260, 80));

        // オーバーレイ線 (60FPS/30FPS)
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 plotPos = ImGui::GetItemRectMin();
        ImVec2 plotSize = ImGui::GetItemRectSize();

        auto drawGuide = [&](float ms, ImU32 color, const char* label) {
            if (ms > maxScale) return;
            float y = plotPos.y + (1.0f - (ms / maxScale)) * plotSize.y;
            dl->AddLine(ImVec2(plotPos.x, y), ImVec2(plotPos.x + plotSize.x, y), color, 1.0f);
            dl->AddText(ImVec2(plotPos.x + 2, y - 12), color, label);
            };
        drawGuide(target60, IM_COL32(100, 255, 100, 200), "60fps");
        drawGuide(target30, IM_COL32(255, 180, 60, 200), "30fps");
        */

        /*
        // ---------- カスタムグラフ描画（上が高い値） ----------
        const ImVec2 graphSize(260, 90);
        ImVec2 canvasMin = ImGui::GetCursorScreenPos();
        ImVec2 canvasMax = ImVec2(canvasMin.x + graphSize.x, canvasMin.y + graphSize.y);
        ImDrawList* draw = ImGui::GetWindowDrawList();

        // 背景
        draw->AddRectFilled(canvasMin, canvasMax, IM_COL32(25, 25, 30, 200), 4.0f);
        draw->AddRect(canvasMin, canvasMax, IM_COL32(200, 200, 200, 90), 4.0f);

        // ガイドライン値
        const float guide60 = target60;   // 16.6ms (60fps)
        const float guide30 = target30;   // 33.3ms (30fps)

        auto msToY = [&](float ms) {
            float t = std::clamp(ms / maxScale, 0.0f, 1.0f);
            // 上が  maxScale, 下が 0
            return canvasMin.y + (1.0f - t) * graphSize.y;
            };

        // ガイドライン描画
        auto drawGuideLine = [&](float ms, ImU32 color, const char* label) {
            if (ms > maxScale) return;
            float y = msToY(ms);
            draw->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), color, 1.0f);
            draw->AddText(ImVec2(canvasMin.x + 4, y - 12), color, label);
            };
        drawGuideLine(guide60, IM_COL32(100, 255, 120, 200), "60fps");
        drawGuideLine(guide30, IM_COL32(255, 190, 80, 200), "30fps");

        // グリッド (等間隔 5 本)
        for (int i = 1; i <= 4; ++i) {
            float y = canvasMin.y + (graphSize.y / 5.0f) * i;
            draw->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), IM_COL32(255, 255, 255, 30), 1.0f);
        }

        // 折れ線
        if (count > 1) {
            const int sampleCount = static_cast<int>(count);
            const float xStep = graphSize.x / float(std::max(sampleCount - 1, 1));
            // start index（リングバッファの最古）
            size_t start = historyFilled_ ? historyIndex_ : 0;
            ImVec2 prev;
            for (int i = 0; i < sampleCount; ++i) {
                size_t idx = (start + i) % kPerfHistoryCount_;
                float ms = frameTimeHistory_[idx];
                float x = canvasMin.x + xStep * i;
                float y = msToY(ms);
                ImVec2 p(x, y);
                if (i > 0) {
                    // スパイクほど色を赤寄りに
                    float norm = std::clamp(ms / maxScale, 0.0f, 1.0f);
                    ImU32 col = ImColor(
                        80 + int(175 * norm),      // R 80→255
                        200 - int(150 * norm),     // G 200→50
                        255 - int(200 * norm),     // B 255→55
                        210);
                    draw->AddLine(prev, p, col, 2.0f);
                }
                prev = p;
            }
        }

        // 最新サンプルを丸でマーク
        if (count > 0) {
            size_t latestIdx = (historyIndex_ + kPerfHistoryCount_ - 1) % kPerfHistoryCount_;
            float latestMs = frameTimeHistory_[latestIdx];
            float t = std::clamp(latestMs / maxScale, 0.0f, 1.0f);
            float x = canvasMax.x;
            float y = msToY(latestMs);
            draw->AddCircleFilled(ImVec2(x, y), 4.0f, IM_COL32(255, 255, 255, 200), 12);
        }

        // 軸ラベル (左端)
        char topLabel[32];  snprintf(topLabel, sizeof(topLabel), "%.1f ms", maxScale);
        char midLabel[32];  snprintf(midLabel, sizeof(midLabel), "%.1f", maxScale * 0.5f);
        draw->AddText(ImVec2(canvasMin.x + 4, canvasMin.y + 2), IM_COL32(200, 200, 200, 180), topLabel);
        draw->AddText(ImVec2(canvasMin.x + 4, canvasMin.y + graphSize.y * 0.5f - 8), IM_COL32(180, 180, 180, 160), midLabel);
        draw->AddText(ImVec2(canvasMin.x + 4, canvasMax.y - 16), IM_COL32(200, 200, 200, 180), "0");

        ImGui::Dummy(graphSize); // レイアウト前進
*/



// 表示モードトグル
static bool showFpsGraph = false;
ImGui::Checkbox("Show FPS graph (instead of Frame Time)", &showFpsGraph);

// 共通パラメータ
const ImVec2 graphSize(260, 90);
ImVec2 canvasMin = ImGui::GetCursorScreenPos();
ImVec2 canvasMax = ImVec2(canvasMin.x + graphSize.x, canvasMin.y + graphSize.y);
ImDrawList* draw = ImGui::GetWindowDrawList();
draw->AddRectFilled(canvasMin, canvasMax, IM_COL32(25, 25, 30, 200), 4.0f);
draw->AddRect(canvasMin, canvasMax, IM_COL32(200, 200, 200, 90), 4.0f);

if (!showFpsGraph) {
    // ---- Frame Time モード (ms) ----
    const float maxScale = std::max(20.0f, cachedMaxMs_ * 1.15f);
    const float guide60 = target60; // 16.7ms
    const float guide30 = target30; // 33.3ms

    auto msToY = [&](float ms) {
        float t = std::clamp(ms / maxScale, 0.0f, 1.0f);
        return canvasMin.y + (1.0f - t) * graphSize.y; // 大きい ms が上
        };
    auto guideLine = [&](float ms, ImU32 col, const char* label) {
        if (ms > maxScale) return;
        float y = msToY(ms);
        draw->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), col, 1.0f);
        draw->AddText(ImVec2(canvasMin.x + 4, y - 12), col, label);
        };
    guideLine(guide60, IM_COL32(100, 255, 120, 200), "60FPS (16.7ms)");
    guideLine(guide30, IM_COL32(255, 190, 80, 200), "30FPS (33.3ms)");

    for (int i = 1; i <= 4; ++i) {
        float y = canvasMin.y + (graphSize.y / 5.0f) * i;
        draw->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), IM_COL32(255, 255, 255, 30), 1.0f);
    }

    if (count > 1) {
        int sampleCount = (int)count;
        float xStep = graphSize.x / float(std::max(sampleCount - 1, 1));
        size_t start = historyFilled_ ? historyIndex_ : 0;
        ImVec2 prev;
        for (int i = 0; i < sampleCount; ++i) {
            size_t idx = (start + i) % kPerfHistoryCount_;
            float ms = frameTimeHistory_[idx];
            float x = canvasMin.x + xStep * i;
            float y = msToY(ms);
            ImVec2 p(x, y);
            if (i > 0) {
                float norm = std::clamp(ms / maxScale, 0.0f, 1.0f);
                ImU32 col = ImColor(
                    80 + int(175 * norm),
                    200 - int(150 * norm),
                    255 - int(200 * norm),
                    210);
                draw->AddLine(prev, p, col, 2.0f);
            }
            prev = p;
        }
        // 最新点
        size_t latestIdx = (historyIndex_ + kPerfHistoryCount_ - 1) % kPerfHistoryCount_;
        float latestMs = frameTimeHistory_[latestIdx];
        draw->AddCircleFilled(ImVec2(canvasMax.x, msToY(latestMs)), 4.0f, IM_COL32(255, 255, 255, 220), 12);
    }

    // 軸ラベル
    char top[32]; snprintf(top, sizeof(top), "%.1f ms (slow)", std::max(0.0f, cachedMaxMs_));
    char mid[32]; snprintf(mid, sizeof(mid), "%.1f", std::max(0.0f, cachedMaxMs_ * 0.5f));
    draw->AddText(ImVec2(canvasMin.x + 4, canvasMin.y + 2), IM_COL32(220, 220, 220, 200), top);
    draw->AddText(ImVec2(canvasMin.x + 4, canvasMin.y + graphSize.y * 0.5f - 8), IM_COL32(200, 200, 200, 160), mid);
    draw->AddText(ImVec2(canvasMin.x + 4, canvasMax.y - 16), IM_COL32(220, 220, 220, 200), "0 ms (fast)");

    ImGui::Dummy(graphSize);
    ImGui::TextUnformatted("Graph: Frame Time (lower is better)");

} else {
    // ---- FPS モード ----
    // フレーム時間履歴を FPS に変換
    static std::vector<float> fpsHistory;
    fpsHistory.resize(count);
    float maxFps = 0.f;
    for (size_t i = 0; i < count; ++i) {
        float ms = frameTimeHistory_[i];
        float f = (ms > 0.0f) ? (1000.0f / ms) : 0.0f;
        fpsHistory[i] = f;
        maxFps = std::max(maxFps, f);
    }
    // 余裕を持たせる
    float fpsScale = std::max(70.0f, maxFps * 1.10f);

    auto fpsToY = [&](float f) {
        float t = std::clamp(f / fpsScale, 0.0f, 1.0f);
        return canvasMin.y + (1.0f - t) * graphSize.y; // 高FPS が上
        };

    auto guideFps = [&](float f, ImU32 col, const char* label) {
        if (f > fpsScale) return;
        float y = fpsToY(f);
        draw->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), col, 1.0f);
        draw->AddText(ImVec2(canvasMin.x + 4, y - 12), col, label);
        };
    guideFps(60.0f, IM_COL32(100, 255, 120, 200), "60FPS");
    guideFps(30.0f, IM_COL32(255, 190, 80, 200), "30FPS");

    for (int i = 1; i <= 4; ++i) {
        float y = canvasMin.y + (graphSize.y / 5.0f) * i;
        draw->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), IM_COL32(255, 255, 255, 30), 1.0f);
    }

    if (count > 1) {
        int sampleCount = (int)count;
        float xStep = graphSize.x / float(std::max(sampleCount - 1, 1));
        size_t start = historyFilled_ ? historyIndex_ : 0;
        ImVec2 prev;
        for (int i = 0; i < sampleCount; ++i) {
            size_t idx = (start + i) % kPerfHistoryCount_;
            float f = fpsHistory[idx];
            float x = canvasMin.x + xStep * i;
            float y = fpsToY(f);
            ImVec2 p(x, y);
            if (i > 0) {
                float norm = std::clamp(f / fpsScale, 0.0f, 1.0f);
                ImU32 col = ImColor(
                    255 - int(150 * norm),     // 低FPSで赤寄り
                    80 + int(170 * norm),
                    100 + int(100 * norm),
                    210);
                draw->AddLine(prev, p, col, 2.0f);
            }
            prev = p;
        }
        // 最新
        size_t latestIdx = (historyIndex_ + kPerfHistoryCount_ - 1) % kPerfHistoryCount_;
        float latestF = fpsHistory[latestIdx];
        draw->AddCircleFilled(ImVec2(canvasMax.x, fpsToY(latestF)), 4.0f, IM_COL32(255, 255, 255, 220), 12);
    }

    char top[32]; snprintf(top, sizeof(top), "%.0f FPS (fast)", fpsScale);
    char mid[32]; snprintf(mid, sizeof(mid), "%.0f", fpsScale * 0.5f);
    draw->AddText(ImVec2(canvasMin.x + 4, canvasMin.y + 2), IM_COL32(220, 220, 220, 200), top);
    draw->AddText(ImVec2(canvasMin.x + 4, canvasMin.y + graphSize.y * 0.5f - 8), IM_COL32(200, 200, 200, 160), mid);
    draw->AddText(ImVec2(canvasMin.x + 4, canvasMax.y - 16), IM_COL32(220, 220, 220, 200), "0 FPS");

    ImGui::Dummy(graphSize);
    ImGui::TextUnformatted("Graph: FPS (higher is better)");
}


        // スパイク検知 (閾値超えフレーム数)
        int spikesOver33 = 0;
        int spikesOver50 = 0;
        for (size_t i = 0; i < count; ++i) {
            if (frameTimeHistory_[i] > 33.3f) ++spikesOver33;
            if (frameTimeHistory_[i] > 50.0f) ++spikesOver50;
        }
        ImGui::Separator();
        ImGui::Text("Spikes >33ms: %d", spikesOver33);
        ImGui::Text("Spikes >50ms: %d", spikesOver50);

        // 詳細トグル
        static bool showRaw = false;
        ImGui::Checkbox("Show raw frame list", &showRaw);
        if (showRaw) {
            if (ImGui::BeginChild("rawFrames", ImVec2(0, 100), true)) {
                for (size_t i = 0; i < count; ++i) {
                    ImGui::Text("%03zu: %.2f ms", i, frameTimeHistory_[i]);
                }
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
}

// ★追加: 統計更新
void DebugUI::UpdatePerfStats_(float newFrameMs) {
    frameTimeHistory_[historyIndex_] = newFrameMs;
    historyIndex_ = (historyIndex_ + 1) % kPerfHistoryCount_;
    if (historyIndex_ == 0) historyFilled_ = true;

    const size_t count = historyFilled_ ? kPerfHistoryCount_ : historyIndex_;
    if (count == 0) return;

    // 基本統計
    float sum = 0.f;
    float mn = FLT_MAX;
    float mx = 0.f;
    for (size_t i = 0; i < count; ++i) {
        float v = frameTimeHistory_[i];
        sum += v;
        mn = std::min(mn, v);
        mx = std::max(mx, v);
    }
    cachedAvgMs_ = sum / static_cast<float>(count);
    cachedMinMs_ = mn;
    cachedMaxMs_ = mx;

    // パーセンタイル (99th frame time → 1% low FPS の近似)
    std::vector<float> sorted;
    sorted.reserve(count);
    for (size_t i = 0; i < count; ++i) sorted.push_back(frameTimeHistory_[i]);
    std::sort(sorted.begin(), sorted.end()); // 昇順 (遅いフレームが後ろ)
    size_t idx99 = static_cast<size_t>(std::clamp(std::floor((sorted.size() - 1) * 0.99f), 0.0f, (float)(sorted.size() - 1)));
    cachedP99Ms_ = sorted[idx99];
}