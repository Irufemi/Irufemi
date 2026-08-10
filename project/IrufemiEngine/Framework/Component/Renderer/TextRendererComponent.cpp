#include "TextRendererComponent.h"
#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Engine/Core/Utility/StringUtility.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Shape/Sphere.h"
#include "Engine/Core/Utility/Log.h"
#include <algorithm>
#include <iostream>

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
    // ロード画面中に生成を終わらせるため、初期化時に強制アップデート（非同期タスク待ち・頂点生成）
    textObj_->Update();
}

void TextRendererComponent::Update() {
    if (!GetTransform() || !textObj_) return;

    // Reflection / Deserialize sync
    std::wstring newText = ConvertString(textU8_);
    if (text_ != newText) {
        SetText(newText);
    }
    if (textObj_->GetFontId() != fontId_) {
        SetFontId(fontId_);
    }
    if (textObj_->GetBaseScale() != baseScale_) {
        SetBaseScale(baseScale_);
    }
    if (textObj_->GetColor() != color_) {
        SetColor(color_);
    }
    if (textObj_->IsTopMost() != isTopMost_) {
        SetTopMost(isTopMost_);
    }
    auto newAlign = static_cast<TextAlignment>(alignmentInt_);
    if (alignment_ != newAlign) {
        SetAlignment(newAlign);
    }

    // Transformの変更をTextオブジェクトに反映
    textObj_->SetPosition(GetTransform()->GetWorldPosition().x, GetTransform()->GetWorldPosition().y, GetTransform()->GetWorldPosition().z);
    textObj_->SetRotation(GetTransform()->GetWorldRotation().z); // 2DなのでZ軸回転
    textObj_->SetScale(GetTransform()->GetWorldScale().x, GetTransform()->GetWorldScale().y);

    textObj_->Update();
}

void TextRendererComponent::Draw() {
    if (isTopMost_) {
        static int debugCount = 0;
        if (debugCount++ % 60 == 0) {
            std::string msg = "[TextRendererComponent] Drawing TopMost Text\n";
            Log::OutPutLog(std::cout, msg);
        }
    }
    textObj_->Draw();
}

bool TextRendererComponent::Raycast(const Irufemi::Ray& ray, float& outDistance) const {
    if (!gameObject_) return false;
    auto transform = GetTransform();
    if (!transform) return false;
    
    // 簡易的にBoundingSphereで判定
    Irufemi::Sphere sphere;
    sphere.center = transform->GetWorldPosition();
    float maxScale = (std::max)({transform->GetWorldScale().x, transform->GetWorldScale().y});
    // Textの横幅は文字数によるため、少し大きめの半径を確保（暫定）
    sphere.radius = maxScale * baseScale_ * (text_.length() * 0.5f); 

    return Irufemi::Collision::IsCollision(ray, sphere, outDistance);
}

void TextRendererComponent::SetText(const std::wstring& text) {
    text_ = text;
    textU8_ = ConvertString(text_);
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

void TextRendererComponent::SetColor(const Irufemi::Vector4& color) {
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
    alignmentInt_ = static_cast<int>(align);
    if (textObj_) {
        textObj_->SetAlignment(alignment_);
    }
}

void TextRendererComponent::OnRegisterProperties() {
    textU8_ = ConvertString(text_);
    alignmentInt_ = static_cast<int>(alignment_);

    RegisterProperty("text", &textU8_);
    RegisterProperty("fontId", &fontId_);
    RegisterProperty("baseScale", &baseScale_);
    RegisterProperty("color", &color_);
    RegisterProperty("alignment", &alignmentInt_);
    RegisterProperty("isTopMost", &isTopMost_);
}


