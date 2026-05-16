#include "TextRendererComponent.h"
#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Engine/Core/Utility/StringUtility.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Shape/Sphere.h"
#include <algorithm>

TextRendererComponent::TextRendererComponent() {}

TextRendererComponent::~TextRendererComponent() {}

void TextRendererComponent::Initialize() {
    textObj_ = std::make_unique<Text>();
    textObj_->Initialize(fontId_);
    textObj_->SetText(text_);
    textObj_->SetBaseScale(baseScale_);
    textObj_->SetColor(color_);
    textObj_->SetTopMost(isTopMost_);
    textObj_->SetAlignment(alignment_);

    transform_ = gameObject_->GetComponent<TransformComponent>();
}

void TextRendererComponent::Update() {
    if (!transform_) return;

    // Transformの変更をTextオブジェクトに反映
    textObj_->SetPosition(transform_->position_.x, transform_->position_.y, transform_->position_.z);
    textObj_->SetRotation(transform_->rotation_.z); // 2DなのでZ軸回転
    textObj_->SetScale(transform_->scale_.x, transform_->scale_.y);

    textObj_->Update();
}

void TextRendererComponent::Draw() {
    textObj_->Draw();
}

bool TextRendererComponent::Raycast(const Ray& ray, float& outDistance) const {
    if (!gameObject_) return false;
    auto transform = gameObject_->GetComponent<TransformComponent>();
    if (!transform) return false;
    
    // 簡易的にBoundingSphereで判定
    Sphere sphere;
    sphere.center = transform->worldPosition_;
    float maxScale = (std::max)({transform->worldScale_.x, transform->worldScale_.y});
    // Textの横幅は文字数によるため、少し大きめの半径を確保（暫定）
    sphere.radius = maxScale * baseScale_ * (text_.length() * 0.5f); 

    return Collision::IsCollision(ray, sphere, outDistance);
}

void TextRendererComponent::SetText(const std::wstring& text) {
    text_ = text;
    if (textObj_) {
        textObj_->SetText(text_);
    }
}

void TextRendererComponent::SetFontId(const std::string& fontId) {
    fontId_ = fontId;
    if (textObj_) {
        textObj_->SetFontId(fontId_);
    }
}

void TextRendererComponent::SetBaseScale(float baseScale) {
    baseScale_ = baseScale;
    if (textObj_) {
        textObj_->SetBaseScale(baseScale_);
    }
}

void TextRendererComponent::SetColor(const Vector4& color) {
    color_ = color;
    if (textObj_) {
        textObj_->SetColor(color_);
    }
}

void TextRendererComponent::SetTopMost(bool isTopMost) {
    isTopMost_ = isTopMost;
    if (textObj_) {
        textObj_->SetTopMost(isTopMost_);
    }
}

void TextRendererComponent::SetAlignment(TextAlignment align) {
    alignment_ = align;
    if (textObj_) {
        textObj_->SetAlignment(alignment_);
    }
}

nlohmann::json TextRendererComponent::Serialize() {
    nlohmann::json j = Component::Serialize();
    // utf-8 std::stringに変換して保存
    j["text"] = ConvertString(text_);
    j["fontId"] = fontId_;
    j["baseScale"] = baseScale_;
    j["color"] = { color_.x, color_.y, color_.z, color_.w };
    j["isTopMost"] = isTopMost_;
    j["alignment"] = static_cast<int>(alignment_);
    return j;
}

void TextRendererComponent::Deserialize(const nlohmann::json& j) {
    Component::Deserialize(j);
    if (j.contains("text")) {
        text_ = ConvertString(j["text"].get<std::string>());
    }
    if (j.contains("fontId")) {
        fontId_ = j["fontId"].get<std::string>();
    }
    if (j.contains("baseScale")) {
        baseScale_ = j["baseScale"].get<float>();
    }
    if (j.contains("color")) {
        auto c = j["color"];
        color_ = { c[0], c[1], c[2], c[3] };
    }
    if (j.contains("isTopMost")) {
        isTopMost_ = j["isTopMost"].get<bool>();
    }
    if (j.contains("alignment")) {
        alignment_ = static_cast<TextAlignment>(j["alignment"].get<int>());
    }

    if (textObj_) {
        textObj_->SetText(text_);
        textObj_->SetFontId(fontId_);
        textObj_->SetBaseScale(baseScale_);
        textObj_->SetColor(color_);
        textObj_->SetTopMost(isTopMost_);
        textObj_->SetAlignment(alignment_);
    }
}
