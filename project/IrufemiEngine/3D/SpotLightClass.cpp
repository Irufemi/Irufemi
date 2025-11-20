#include "SpotLightClass.h"

#include <imgui.h>
#include "function/Math.h"
#include <cmath>
#include <numbers>

DirectXCommon* SpotLightClass::dxCommon_ = nullptr;

void SpotLightClass::Initialize() {

    resource_ = dxCommon_->CreateBufferResource(sizeof(SpotLight));
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
    // デフォルト
    data_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    data_->position = { 2.0f, 1.25f, 0.0f };
    data_->distance = 7.0f;
    data_->direction = Math::Normalize({ -1.0f,-1.0f,0.0f });
    data_->intensity = 4.0f;
    data_->decay = 2.0f;
    data_->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
}

void SpotLightClass::Debug() {

#ifdef USE_IMGUI

    if (ImGui::CollapsingHeader("SpotLight")) {
        ImGui::ColorEdit4("SpotLightColor", &data_->color.x);
        ImGui::DragFloat3("SpotLightPosition", &data_->position.x, 0.01f);
        ImGui::DragFloat("SpotLightDistance", &data_->distance, 0.01f, 0.0f);
        // 方向ベクトルは都度正規化
        if (ImGui::DragFloat3("SpotLightDirection", &data_->direction.x, 0.01f)) {
            data_->direction = Math::Normalize(data_->direction);
        }
        ImGui::DragFloat("SpotLightIntensity", &data_->intensity, 0.01f, 0.0f);
        ImGui::DragFloat("SpotLightDecay", &data_->decay, 0.01f, 0.0f);
        ImGui::DragFloat("SpotLightCosAngle", &data_->cosAngle, 0.01f);
    }

#endif // USE_IMGUI


}