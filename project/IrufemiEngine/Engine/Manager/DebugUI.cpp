#define NOMINMAX
#include "DebugUI.h"
#include <Windows.h>

/*開発のUIを出そう*/

#ifdef USE_IMGUI
#include "imgui/imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
// ImGuiWindowFlags_NoDocking が未定義の場合は定義する
#ifndef ImGuiWindowFlags_NoDocking
#define ImGuiWindowFlags_NoDocking (1 << 13)
#endif
#endif // USE_IMGUI
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <numeric> 
#include "Resource/Texture/TextureManager.h"
#include "Framework/SceneManager.h"
#include "Engine/Core/Shape/Sphere.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Graphics/Data/DirectionalLight.h"
#include "Engine/Graphics/Data/PointLight.h"
#include "Engine/Graphics/Data/SpotLight.h"
#include "Engine/Graphics/Data/AreaLight.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/DirectX/DescriptorPool.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/Material.h"
#include "Renderer/Particle/Data/ParticleMaterial.h"
#include "Resource/Model/Data/ObjModel.h"
#include "Renderer/D3D12ResourceUtil.h"
#include "Engine/Core/Math/Geometry/Math.h"

// 静的宣言
std::unique_ptr<PointLight> DebugUI::templatePointLight_;
std::unique_ptr<SpotLight> DebugUI::templateSpotLight_;
std::unique_ptr<AreaLight> DebugUI::templateAreaLight_;

void DebugUI::Initialize([[maybe_unused]] HWND hwnd, [[maybe_unused]] DirectXCommon* dxCommon) {
#ifdef USE_IMGUI

    dxCommon_ = dxCommon;

    /*開発UIを出そう*/
    //ImGuiの初期化。詳細はさして重要ではないので開設は省略する。
    //こういうもんである
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);

    DescriptorPool* srvPool = dxCommon->GetSrvPool();
    ID3D12DescriptorHeap* srvHeap = srvPool->GetHeap();

    // ImGui用にディスクリプタを1つ確保
    const uint32_t imguiIndex = srvPool->Allocate();
    assert(imguiIndex != DescriptorPool::kInvalid);

    ImGui_ImplDX12_Init(
        dxCommon->GetDevice(),
        dxCommon->GetSwapChainDesc().BufferCount,
        dxCommon->GetRtvDesc().Format,
        srvHeap,
        srvPool->GetCPUHandle(imguiIndex),
        srvPool->GetGPUHandle(imguiIndex)
    );

    // テンプレートライトの初期化
    templatePointLight_ = std::make_unique<PointLight>();
    templatePointLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    templatePointLight_->position = { 0.0f, 1.0f, 0.0f };
    templatePointLight_->intensity = 1.0f;
    templatePointLight_->radius = 10.0f;
    templatePointLight_->decay = 1.0f;
    templatePointLight_->isActive = 1;

    templateSpotLight_ = std::make_unique<SpotLight>();
    templateSpotLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    templateSpotLight_->position = { 0.0f, 1.0f, 0.0f };
    templateSpotLight_->distance = 10.0f;
    templateSpotLight_->direction = { 0.0f, -1.0f, 0.0f };
    templateSpotLight_->intensity = 1.0f;
    templateSpotLight_->decay = 1.0f;
    templateSpotLight_->cosAngle = std::cos(std::numbers::pi_v<float> / 6.0f);
    templateSpotLight_->isActive = 1;

    templateAreaLight_ = std::make_unique<AreaLight>();
    templateAreaLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    templateAreaLight_->position = { 0.0f, 1.0f, 0.0f };
    templateAreaLight_->intensity = 1.0f;
    templateAreaLight_->direction = { 0.0f, -1.0f, 0.0f };
    templateAreaLight_->range = 10.0f;
    templateAreaLight_->size = { 1.0f, 1.0f };
    templateAreaLight_->isActive = 1;

#endif // USE_IMGUI
}

void DebugUI::FrameStart() {

#ifdef USE_IMGUI

    /*開発のUIを出そう*/

    ///ImGuiを使う
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

#endif // USE_IMGUI
}

void DebugUI::Shutdown() {
#ifdef USE_IMGUI


    /*開発のUIを出そう*/

    ///ImGuiの終了処理

    //ImGuiの終了処理。詳細はさして重要ではないので解説は省略する。
    //こういうもんである。初期化と逆順に行う。
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

#endif // USE_IMGUI
}
#ifdef USE_IMGUI

LRESULT DebugUI::WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {


    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
        return TRUE;
    }

    return FALSE;

}
#endif // USE_IMGUI



void DebugUI::QueueDrawCommands() {
#ifdef USE_IMGUI


    /*開発のUIを出そう*/

    ///ImGuiを使う

    //ImGuiの内部コマンドを生成する
    ImGui::Render();
#endif // USE_IMGUI
}

void DebugUI::QueuePostDrawCommands() {
#ifdef USE_IMGUI


    /*開発のUIを出そう*/
/*開発のUIを出そう*/

    ///ImGuiを描画する

    //描画用のDescriptorHeapの設定
    ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
    dxCommon_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);

    ///ImGuiを描画する

    //実際のcommandListのImGuiの描画コマンドを積む
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon_->GetCommandList());

#endif // USE_IMGUI
}

void DebugUI::DebugLights(
    [[maybe_unused]] DirectionalLight* directionalLight,
    [[maybe_unused]] std::vector<std::unique_ptr<PointLight>>& pointLights,
    [[maybe_unused]] std::vector<std::unique_ptr<SpotLight>>& spotLights,
    [[maybe_unused]] std::vector<std::unique_ptr<AreaLight>>& areaLights) {
#ifdef USE_IMGUI
    // Light Editor タブ
    if (ImGui::BeginTabItem("Light Editor")) {
        ImGui::SeparatorText("PointLight Template");
        ImGui::ColorEdit4("PL Color", &templatePointLight_->color.x);
        ImGui::DragFloat3("PL Position", &templatePointLight_->position.x, 0.01f);
        ImGui::DragFloat("PL Intensity", &templatePointLight_->intensity, 0.01f, 0.0f);
        ImGui::DragFloat("PL Radius", &templatePointLight_->radius, 0.01f, 0.0f);
        ImGui::DragFloat("PL Decay", &templatePointLight_->decay, 0.01f, 0.0f);
        if (ImGui::Button("Add PointLight to Scene")) {
            auto newLight = std::make_unique<PointLight>(*templatePointLight_);
            pointLights.push_back(std::move(newLight));
        }

        ImGui::Separator();

        ImGui::SeparatorText("SpotLight Template");
        ImGui::ColorEdit4("SL Color", &templateSpotLight_->color.x);
        ImGui::DragFloat3("SL Position", &templateSpotLight_->position.x, 0.01f);
        ImGui::DragFloat("SL Intensity", &templateSpotLight_->intensity, 0.01f, 0.0f);
        ImGui::DragFloat3("SL Direction", &templateSpotLight_->direction.x, 0.01f);
        templateSpotLight_->direction = Math::Normalize(templateSpotLight_->direction);
        ImGui::DragFloat("SL Distance", &templateSpotLight_->distance, 0.01f, 0.0f);
        ImGui::DragFloat("SL Decay", &templateSpotLight_->decay, 0.01f, 0.0f);
        ImGui::DragFloat("SL CosAngle", &templateSpotLight_->cosAngle, 0.01f, 0.0f, 1.0f);
        if (ImGui::Button("Add SpotLight to Scene")) {
            auto newLight = std::make_unique<SpotLight>(*templateSpotLight_);
            spotLights.push_back(std::move(newLight));
        }

        ImGui::Separator();

        ImGui::SeparatorText("AreaLight Template");
        ImGui::ColorEdit4("AL Color", &templateAreaLight_->color.x);
        ImGui::DragFloat3("AL Position", &templateAreaLight_->position.x, 0.01f);
        ImGui::DragFloat("AL Intensity", &templateAreaLight_->intensity, 0.01f, 0.0f);
        ImGui::DragFloat3("AL Direction", &templateAreaLight_->direction.x, 0.01f);
        templateAreaLight_->direction = Math::Normalize(templateAreaLight_->direction);
        ImGui::DragFloat("AL Range", &templateAreaLight_->range, 0.01f, 0.0f);
        ImGui::DragFloat2("AL Size", &templateAreaLight_->size.x, 0.01f, 0.0f);
        if (ImGui::Button("Add AreaLight to Scene")) {
            auto newLight = std::make_unique<AreaLight>(*templateAreaLight_);
            areaLights.push_back(std::move(newLight));
        }

        ImGui::EndTabItem();
    }

    // DirectionalLight タブ
    if (directionalLight && ImGui::BeginTabItem("DirectionalLight")) {
        ImGui::ColorEdit4("Color", &directionalLight->color.x);
        ImGui::DragFloat3("Direction", &directionalLight->direction.x, 0.01f);
        directionalLight->direction = Math::Normalize(directionalLight->direction);
        ImGui::DragFloat("Intensity", &directionalLight->intensity, 0.01f, 0.0f);
        ImGui::EndTabItem();
    }

    // PointLights タブ
    if (ImGui::BeginTabItem("PointLights")) {
        int pointLightToRemove = -1;
        for (size_t i = 0; i < pointLights.size(); ++i) {
            auto& light = pointLights[i];
            std::string label = "PointLight " + std::to_string(i);
            if (ImGui::CollapsingHeader(label.c_str())) {
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Button("[-] Remove")) {
                    pointLightToRemove = static_cast<int>(i);
                }
                ImGui::SameLine();
                ImGui::Checkbox("IsActive", reinterpret_cast<bool*>(&light->isActive));
                ImGui::ColorEdit4("Color", &light->color.x);
                ImGui::DragFloat3("Position", &light->position.x, 0.01f);
                ImGui::DragFloat("Intensity", &light->intensity, 0.01f, 0.0f);
                ImGui::DragFloat("Radius", &light->radius, 0.01f, 0.0f);
                ImGui::DragFloat("Decay", &light->decay, 0.01f, 0.0f);
                ImGui::PopID();
            }
        }
        if (pointLightToRemove != -1) {
            pointLights.erase(pointLights.begin() + pointLightToRemove);
        }
        ImGui::EndTabItem();
    }

    // SpotLights タブ
    if (ImGui::BeginTabItem("SpotLights")) {
        int spotLightToRemove = -1;
        for (size_t i = 0; i < spotLights.size(); ++i) {
            auto& light = spotLights[i];
            std::string label = "SpotLight " + std::to_string(i);
            if (ImGui::CollapsingHeader(label.c_str())) {
                ImGui::PushID(static_cast<int>(i + pointLights.size()));
                if (ImGui::Button("[-] Remove")) {
                    spotLightToRemove = static_cast<int>(i);
                }
                ImGui::SameLine();
                ImGui::Checkbox("IsActive", reinterpret_cast<bool*>(&light->isActive));
                ImGui::ColorEdit4("Color", &light->color.x);
                ImGui::DragFloat3("Position", &light->position.x, 0.01f);
                ImGui::DragFloat("Intensity", &light->intensity, 0.01f, 0.0f);
                ImGui::DragFloat3("Direction", &light->direction.x, 0.01f);
                light->direction = Math::Normalize(light->direction);
                ImGui::DragFloat("Distance", &light->distance, 0.01f, 0.0f);
                ImGui::DragFloat("Decay", &light->decay, 0.01f, 0.0f);
                ImGui::DragFloat("CosAngle", &light->cosAngle, 0.01f, 0.0f, 1.0f);
                ImGui::PopID();
            }
        }
        if (spotLightToRemove != -1) {
            spotLights.erase(spotLights.begin() + spotLightToRemove);
        }
        ImGui::EndTabItem();
    }

    // AreaLights タブ
    if (ImGui::BeginTabItem("AreaLights")) {
        int areaLightToRemove = -1;
        for (size_t i = 0; i < areaLights.size(); ++i) {
            auto& light = areaLights[i];
            std::string label = "AreaLight " + std::to_string(i);
            if (ImGui::CollapsingHeader(label.c_str())) {
                ImGui::PushID(static_cast<int>(i + pointLights.size() + spotLights.size()));
                if (ImGui::Button("[-] Remove")) {
                    areaLightToRemove = static_cast<int>(i);
                }
                ImGui::SameLine();
                ImGui::Checkbox("IsActive", reinterpret_cast<bool*>(&light->isActive));
                ImGui::ColorEdit4("Color", &light->color.x);
                ImGui::DragFloat3("Position", &light->position.x, 0.01f);
                ImGui::DragFloat("Intensity", &light->intensity, 0.01f, 0.0f);
                ImGui::DragFloat3("Direction", &light->direction.x, 0.01f);
                light->direction = Math::Normalize(light->direction);
                ImGui::DragFloat("Range", &light->range, 0.01f, 0.0f);
                ImGui::DragFloat2("Size", &light->size.x, 0.01f, 0.0f);
                ImGui::PopID();
            }
        }
        if (areaLightToRemove != -1) {
            areaLights.erase(areaLights.begin() + areaLightToRemove);
        }
        ImGui::EndTabItem();
    }

#endif
}

// transform
void DebugUI::DebugTransform([[maybe_unused]] Transform& transform) {
#ifdef USE_IMGUI

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
#endif // USE_IMGUI
}

// transform
void DebugUI::DebugTransform2D([[maybe_unused]] Transform& transform) {
#ifdef USE_IMGUI

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
#endif // USE_IMGUI
}

void DebugUI::TextTransform([[maybe_unused]] Transform& transform, [[maybe_unused]] const char* name) {
#ifdef USE_IMGUI

    std::string header = std::string("transform") + name;
    if (ImGui::CollapsingHeader(header.c_str())) {
        ImGui::Text("scale: (%.2f, %.2f, %.2f)", transform.scale.x, transform.scale.y, transform.scale.z);
        ImGui::Text("rotate: (%.2f, %.2f, %.2f)", transform.rotate.x, transform.rotate.y, transform.rotate.z);
        ImGui::Text("translate: (%.2f, %.2f, %.2f)", transform.translate.x, transform.translate.y, transform.translate.z);
    }
#endif // USE_IMGUI
}

// ObjMaterial
void DebugUI::DebugObjMaterial([[maybe_unused]] ObjMaterial* material, [[maybe_unused]] const char* unique_id) {
#ifdef USE_IMGUI
    if (!material) return;

    std::string id_str = unique_id;

    ImGui::ColorEdit4(("Color" + id_str).c_str(), &material->color.x);
    ImGui::Checkbox(("Enable Lighting" + id_str).c_str(), &material->enableLighting);
    ImGui::DragFloat(("Shininess" + id_str).c_str(), &material->shininess, 1.0f, 1.0f, 256.0f);
    ImGui::DragFloat(("Environment Coefficient" + id_str).c_str(), &material->environmentCoefficient, 0.01f, 0.0f, 1.0f);

    // UV Transform
    if (ImGui::TreeNode(("UV Transform" + id_str).c_str())) {
        DebugUvTransform(material->uvTransform);
        ImGui::TreePop();
    }
#endif // USE_IMGUI
}

// Material
void DebugUI::DebugMaterialBy3D([[maybe_unused]] Material* materialData) {
#ifdef USE_IMGUI

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
        //ImGui::DragFloat("Environment Coefficient", &materialData->environmentCoefficient, 0.01f, 0.0f, 1.0f);
    }
#endif // USE_IMGUI
}

// Material
void DebugUI::DebugMaterialBy2D([[maybe_unused]] Material* materialData) {
#ifdef USE_IMGUI

    if (ImGui::CollapsingHeader("material")) {
        ImGui::ColorEdit4("spriteColor", &materialData->color.x);
    }
#endif // USE_IMGUI
}

// Particle 専用マテリアルのデバッグ表示
void DebugUI::DebugMaterialParticle([[maybe_unused]] ParticleMaterial* materialData) {
#ifdef USE_IMGUI

    if (!materialData) return;

    if (ImGui::CollapsingHeader("particle material")) {
        // 基本プロパティ
        ImGui::ColorEdit4("color", &materialData->color.x);

        // サンプラ切替フラグ(0 = WRAP(s0), 1 = CLAMP(s1))
        bool useClamp = materialData->useClampSampler != 0;
        if (ImGui::Checkbox("Use Clamp Sampler (V)", &useClamp)) {
            materialData->useClampSampler = useClamp ? 1 : 0;
        }

        // --- UV Transform 編集(より実用的に) ---
        // materialData->uvTransform は 4x4 行列。
        // 編集用に translate/scale/rotate(Z) を抽出し、編集後に再構成する。
        // 抽出は「一般的な affine(回転 + scale + translate)を想定した簡易逆変換」です。
        // U/V は X/Y 成分に対応している前提。
        float tx = materialData->uvTransform.m[3][0];
        float ty = materialData->uvTransform.m[3][1];

        // 簡易スケール抽出：対角成分を利用(斜交/shear を無視する簡易推定)
        float sx = materialData->uvTransform.m[0][0];
        float sy = materialData->uvTransform.m[1][1];

        // 簡易回転(ラジアン)： atan2( m10, m00 ) を使用(回転+scale の混在を近似)
        float rot = std::atan2(materialData->uvTransform.m[1][0], materialData->uvTransform.m[0][0]);

        bool changed = false;
        if (ImGui::TreeNode("UV Transform (affine)")) {
            if (ImGui::DragFloat2("UV Translate", &tx, 0.01f, -100.0f, 100.0f)) changed = true;
            if (ImGui::DragFloat2("UV Scale", &sx, 0.01f, -100.0f, 100.0f)) changed = true;
            if (ImGui::SliderAngle("UV Rotate (deg)", &rot)) changed = true;
            ImGui::TextWrapped("注: 複雑な歪み(shear 等)がある場合は完璧に逆変換できません。一般的な UV 編集用途に最適化しています。");
            ImGui::TreePop();
        }

        if (changed) {
            // Transform 構造を使って行列を再構成(function/Math.h の MakeAffineMatrix を利用)
            Transform uvT;
            uvT.translate = { tx, ty, 0.0f };
            uvT.scale = { sx, sy, 1.0f };
            uvT.rotate = { 0.0f, 0.0f, rot }; // rad

            materialData->uvTransform = Math::MakeAffineMatrix(uvT.scale, uvT.rotate, uvT.translate);
        }
    }
#endif // USE_IMGUI
}

// 画像
void DebugUI::DebugTexture([[maybe_unused]] D3D12ResourceUtil* resource, [[maybe_unused]] int& selectedTextureIndex) {
#ifdef USE_IMGUI

    if (textureManager_) {
        auto textureNames = textureManager_->GetTextureNames();
        std::sort(textureNames.begin(), textureNames.end());
        if (!textureNames.empty()) {
            const char* preview = textureNames[selectedTextureIndex].c_str();
            if (ImGui::BeginCombo("Texture", preview)) {
                for (int i = 0; i < static_cast<int>(textureNames.size()); ++i) {
                    bool isSelected = (i == selectedTextureIndex);
                    if (ImGui::Selectable(textureNames[i].c_str(), isSelected)) {
                        selectedTextureIndex = i;
                        resource->textureHandle_ = textureManager_->GetTextureHandle(textureNames[i]);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
    }
#endif // USE_IMGUI
}

void DebugUI::DebugTexture([[maybe_unused]] D3D12ResourceUtilParticle* resource, [[maybe_unused]] int& selectedTextureIndex) {
#ifdef USE_IMGUI

    if (textureManager_) {
        auto textureNames = textureManager_->GetTextureNames();
        std::sort(textureNames.begin(), textureNames.end());
        if (!textureNames.empty()) {
            const char* preview = textureNames[selectedTextureIndex].c_str();
            if (ImGui::BeginCombo("Texture", preview)) {
                for (int i = 0; i < static_cast<int>(textureNames.size()); ++i) {
                    bool isSelected = (i == selectedTextureIndex);
                    if (ImGui::Selectable(textureNames[i].c_str(), isSelected)) {
                        selectedTextureIndex = i;
                        resource->textureHandle_ = textureManager_->GetTextureHandle(textureNames[i]);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
    }
#endif // USE_IMGUI
}

// DirectionalLight
void DebugUI::DebugDirectionalLight([[maybe_unused]] DirectionalLight* directionalLightData) {
#ifdef USE_IMGUI

    if (ImGui::CollapsingHeader("directionalLight")) {
        ImGui::ColorEdit4("lightColor", &directionalLightData->color.x);
        ImGui::DragFloat3("lightDirection", &directionalLightData->direction.x, 0.01f);
        ImGui::DragFloat("intensity", &directionalLightData->intensity, 0.01f, 0.0f);
    }
#endif // USE_IMGUI
}

// UvTransform
void DebugUI::DebugUvTransform([[maybe_unused]] Transform& uvTransform) {
#ifdef USE_IMGUI

    if (ImGui::CollapsingHeader("uvTransform")) {
        ImGui::DragFloat3("UVTranslate", &uvTransform.translate.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat3("UVScale", &uvTransform.scale.x, 0.01f, -10.0f, 10.0f);
        ImGui::SliderAngle("UVRotate", &uvTransform.rotate.z);
    }
#endif // USE_IMGUI
}

// UvTransform
void DebugUI::DebugUvTransform([[maybe_unused]] Matrix4x4& uvTransform) {
#ifdef USE_IMGUI
    if (ImGui::CollapsingHeader("uvTransform")) {
        // 編集用に translate/scale/rotate(Z) を抽出
        float tx = uvTransform.m[3][0];
        float ty = uvTransform.m[3][1];
        float sx = std::sqrt(uvTransform.m[0][0] * uvTransform.m[0][0] + uvTransform.m[0][1] * uvTransform.m[0][1]);
        float sy = std::sqrt(uvTransform.m[1][0] * uvTransform.m[1][0] + uvTransform.m[1][1] * uvTransform.m[1][1]);
        float rot = std::atan2(uvTransform.m[1][0], uvTransform.m[0][0]);

        bool changed = false;
        if (ImGui::DragFloat2("UVTranslate", &tx, 0.01f)) changed = true;
        if (ImGui::DragFloat2("UVScale", &sx, 0.01f)) {
            sy = sx; // XとYを同じ値に保つ
            changed = true;
        }
        if (ImGui::SliderAngle("UVRotate", &rot)) changed = true;

        if (changed) {
            // Transform 構造を使って行列を再構成
            Transform uvT;
            uvT.translate = { tx, ty, 0.0f };
            uvT.scale = { sx, sy, 1.0f };
            uvT.rotate = { 0.0f, 0.0f, rot }; // rad
            uvTransform = Math::MakeAffineMatrix(uvT.scale, uvT.rotate, uvT.translate);
        }
    }
#endif // USE_IMGUI
}
// Sphere
void DebugUI::DebugSphereInfo([[maybe_unused]] Sphere& sphere) {
#ifdef USE_IMGUI

    if (ImGui::CollapsingHeader("info")) {
        ImGui::DragFloat3("Center", &sphere.center.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat("radius", &sphere.radius, 0.01f, -10.0f, 10.0f);
    }
#endif // USE_IMGUI
}

// FPS/FrameTime オーバーレイ
void DebugUI::FPSDebug() {
#ifdef USE_IMGUI

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
        // ---------- カスタムグラフ描画(上が高い値) ----------
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
        drawGuideLine(guide30, IM_COL32(255,190, 80, 200), "30fps");

        // グリッド (等間隔 5 本)
        for (int i = 1; i <= 4; ++i) {
            float y = canvasMin.y + (graphSize.y / 5.0f) * i;
            draw->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), IM_COL32(255, 255, 255, 30), 1.0f);
        }

        // 折れ線
        if (count > 1) {
            const int sampleCount = static_cast<int>(count);
            const float xStep = graphSize.x / float(std::max(sampleCount - 1, 1));
            // start index(リングバッファの最古)
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
#endif // USE_IMGUI
}

// ★追加: 統計更新
void DebugUI::UpdatePerfStats_([[maybe_unused]] float newFrameMs) {
#ifdef USE_IMGUI

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
#endif // USE_IMGUI
}

void DebugUI::DebugSceneSelector([[maybe_unused]] SceneManager* sm) {
#ifdef USE_IMGUI

    if (!sm) { return; }

    const auto names = sm->GetRegisteredKeys();
    if (names.empty()) { return; }

    // 現在シーンのインデックス
    int currentIdx = 0;
    for (int i = 0; i < static_cast<int>(names.size()); ++i) {
        if (names[i] == sm->GetCurrent()) { currentIdx = i; break; }
    }

    ImGui::Begin("Scene Selector");
    if (ImGui::BeginCombo("Scene", names[currentIdx].c_str())) {
        for (int i = 0; i < static_cast<int>(names.size()); ++i) {
            bool selected = (i == currentIdx);
            if (ImGui::Selectable(names[i].c_str(), selected)) {
                sm->Request(names[i]); // 次フレーム頭で切替
            }
            if (selected) { ImGui::SetItemDefaultFocus(); }
        }
        ImGui::EndCombo();
    }
    ImGui::End();
#endif // USE_IMGUI
}

void DebugUI::DebugPsoSettings(
    [[maybe_unused]] BlendMode* blendMode,
    [[maybe_unused]] PSOManager::DepthWrite* depthWrite,
    [[maybe_unused]] PSOManager::CullMode* cullMode,
    [[maybe_unused]] const char* unique_id) {
#ifdef USE_IMGUI
    if (!blendMode || !depthWrite || !cullMode) {
        return;
    }

    // Blend Mode
    int blendIdx = static_cast<int>(*blendMode);
    const char* blendNames[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
    std::string blendLabel = "Blend Mode";
    blendLabel += unique_id;
    if (ImGui::Combo(blendLabel.c_str(), &blendIdx, blendNames, IM_ARRAYSIZE(blendNames))) {
        *blendMode = static_cast<BlendMode>(blendIdx);
    }

    // Depth Write
    int depthIdx = (*depthWrite == PSOManager::DepthWrite::Enable) ? 0 : 1;
    const char* depthNames[] = { "Enable", "Disable" };
    std::string depthLabel = "Depth Write";
    depthLabel += unique_id;
    if (ImGui::Combo(depthLabel.c_str(), &depthIdx, depthNames, IM_ARRAYSIZE(depthNames))) {
        *depthWrite = (depthIdx == 0) ? PSOManager::DepthWrite::Enable : PSOManager::DepthWrite::Disable;
    }

    // Cull Mode
    int cullIdx = static_cast<int>(*cullMode);
    const char* cullNames[] = { "Back", "Front", "None" };
    std::string cullLabel = "Cull Mode";
    cullLabel += unique_id;
    if (ImGui::Combo(cullLabel.c_str(), &cullIdx, cullNames, IM_ARRAYSIZE(cullNames))) {
        *cullMode = static_cast<PSOManager::CullMode>(cullIdx);
    }
#endif // USE_IMGUI
}

void DebugUI::DebugPostProcess([[maybe_unused]] IrufemiEngine* engine) {
#ifdef USE_IMGUI
    if (!engine) return;

    ImGui::Begin("Post Processing");

    int currentMode = static_cast<int>(engine->GetPostProcessMode());
    const char* modes[] = { "None", "Grayscale", "Sepia", "Vignette" };

    if (ImGui::Combo("Mode", &currentMode, modes, IM_ARRAYSIZE(modes))) {
        engine->SetPostProcessMode(static_cast<IrufemiEngine::PostProcessMode>(currentMode));
    }

    if (engine->GetPostProcessMode() == IrufemiEngine::PostProcessMode::Vignette) {
        auto& params = engine->GetVignetteParams();
        ImGui::DragFloat("Vignette Scale", &params.scale, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Vignette Power", &params.power, 0.01f, 0.0f, 10.0f);
    }

    ImGui::End();
#endif // USE_IMGUI
}

