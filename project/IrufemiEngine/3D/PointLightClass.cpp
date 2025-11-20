#include "PointLightClass.h"
#include <imgui.h>

DirectXCommon* PointLightClass::dxCommon_ = nullptr;

void PointLightClass::Initialize() {

    resource_ = dxCommon_->CreateBufferResource(sizeof(PointLight));
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
    // デフォルト
    data_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    data_->position = { 0.0f, 5.0f, 0.0f };
    data_->intensity = 1.0f;
}

void PointLightClass::Debug() {
#ifdef USE_IMGUI

    if (ImGui::CollapsingHeader("PointLight")) {
        ImGui::ColorEdit4("PointLightColor", &data_->color.x);
        ImGui::DragFloat3("PointLightPosition", &data_->position.x, 0.01f);
        ImGui::DragFloat("PointLightIntensity", &data_->intensity, 0.01f, 0.0f);
    }

#endif // USE_IMGUI
}