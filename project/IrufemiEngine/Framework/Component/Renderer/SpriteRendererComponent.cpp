#include "SpriteRendererComponent.h"
#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Resource/Texture/TextureManager.h"

#ifdef EditorMode
#include "imgui/imgui.h"
#endif

SpriteRendererComponent::SpriteRendererComponent() {}
SpriteRendererComponent::~SpriteRendererComponent() {}

void SpriteRendererComponent::Initialize() {
    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(texturePath_);
    
    // 初期設定
    sprite_->SetAnchor(anchor_[0], anchor_[1]);
    sprite_->SetFlip(isFlipX_, isFlipY_);
    sprite_->SetTopMost(isTopMost_);
    sprite_->SetColor(color_);
    
    // テクスチャサイズを取得して初期サイズに設定
    size_[0] = sprite_->GetSize().x;
    size_[1] = sprite_->GetSize().y;

    if (gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
    }
}

void SpriteRendererComponent::Update() {
    if (transform_ && sprite_) {
        // SpriteはZ位置も保持できるが基本は2D
        sprite_->SetPosition(transform_->worldPosition_.x, transform_->worldPosition_.y, transform_->worldPosition_.z);
        // Spriteの回転はZ軸のみ
        sprite_->SetRotation(transform_->worldRotation_.z);
        
        // TransformのScaleは、SpriteのBaseサイズに対するスケーリングとして扱う
        sprite_->SetSize(size_[0] * transform_->worldScale_.x, size_[1] * transform_->worldScale_.y);
    }

    if (sprite_) {
        sprite_->Update();
    }
}

void SpriteRendererComponent::Draw() {
    if (sprite_) {
        sprite_->Draw(); // SyncBeforeDrawはSprite内で呼ばれる
    }
}

void SpriteRendererComponent::SetTexture(const std::string& texturePath) {
    texturePath_ = texturePath;
    if (sprite_) {
        sprite_->SetTexture(texturePath_);
        // テクスチャ変更に合わせてサイズを更新
        size_[0] = sprite_->GetSize().x;
        size_[1] = sprite_->GetSize().y;
    }
}

void SpriteRendererComponent::OnInspectorGUI() {
#ifdef EditorMode
    if (ImGui::TreeNodeEx("SpriteRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (sprite_) {
            bool needUpdate = false;
            
            // テクスチャ選択UIをコンポーネント側で完全に構築する
            TextureManager* tm = Sprite::GetTextureManager();
            if (tm) {
                auto names = tm->GetTextureNamesForDebug();
                int currentIndex = 0;
                for (int i = 0; i < names.size(); ++i) {
                    if (names[i] == texturePath_) {
                        currentIndex = i;
                        break;
                    }
                }
                const char* currentPreview = names.empty() ? "" : names[currentIndex].c_str();
                if (ImGui::BeginCombo("Texture", currentPreview)) {
                    for (int i = 0; i < names.size(); ++i) {
                        bool isSelected = (currentIndex == i);
                        if (ImGui::Selectable(names[i].c_str(), isSelected)) {
                            texturePath_ = names[i];
                            SetTexture(texturePath_);
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            } else {
                // TextureManagerが無い場合のフォールバック
                char buffer[256];
                strncpy_s(buffer, texturePath_.c_str(), sizeof(buffer) - 1);
                if (ImGui::InputText("TexturePath", buffer, sizeof(buffer))) {
                    texturePath_ = buffer;
                    SetTexture(texturePath_);
                }
            }
            
            if (ImGui::Checkbox("TopMost (Draw over 3D)", &isTopMost_)) {
                sprite_->SetTopMost(isTopMost_);
            }
            if (ImGui::Checkbox("Flip X", &isFlipX_)) needUpdate = true;
            ImGui::SameLine();
            if (ImGui::Checkbox("Flip Y", &isFlipY_)) needUpdate = true;
            
            if (needUpdate) {
                sprite_->SetFlip(isFlipX_, isFlipY_);
            }

            if (ImGui::SliderFloat2("Anchor", anchor_, 0.0f, 1.0f)) {
                sprite_->SetAnchor(anchor_[0], anchor_[1]);
            }
            if (ImGui::DragFloat2("Base Size", size_, 1.0f, 1.0f, 8192.0f)) {
                // サイズはUpdate内で適用される
            }
            
            if (ImGui::ColorEdit4("Color", &color_.x)) {
                sprite_->SetColor(color_);
            }
        }
        ImGui::TreePop();
    }
#endif
}

nlohmann::json SpriteRendererComponent::Serialize() {
    nlohmann::json j;
    j["texturePath"] = texturePath_;
    j["isTopMost"] = isTopMost_;
    j["isFlipX"] = isFlipX_;
    j["isFlipY"] = isFlipY_;
    j["anchor"] = { anchor_[0], anchor_[1] };
    j["size"] = { size_[0], size_[1] };
    j["color"] = { color_.x, color_.y, color_.z, color_.w };
    return j;
}

void SpriteRendererComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("texturePath")) SetTexture(j["texturePath"]);
    if (j.contains("isTopMost")) isTopMost_ = j["isTopMost"];
    if (j.contains("isFlipX")) isFlipX_ = j["isFlipX"];
    if (j.contains("isFlipY")) isFlipY_ = j["isFlipY"];
    if (j.contains("anchor") && j["anchor"].is_array() && j["anchor"].size() == 2) {
        anchor_[0] = j["anchor"][0];
        anchor_[1] = j["anchor"][1];
    }
    if (j.contains("size") && j["size"].is_array() && j["size"].size() == 2) {
        size_[0] = j["size"][0];
        size_[1] = j["size"][1];
    }
    if (j.contains("color") && j["color"].is_array() && j["color"].size() == 4) {
        color_.x = j["color"][0];
        color_.y = j["color"][1];
        color_.z = j["color"][2];
        color_.w = j["color"][3];
    }
    
    // 反映
    if (sprite_) {
        sprite_->SetAnchor(anchor_[0], anchor_[1]);
        sprite_->SetFlip(isFlipX_, isFlipY_);
        sprite_->SetTopMost(isTopMost_);
        sprite_->SetColor(color_);
        // サイズの反映はUpdateでscaleを考慮して行われるが、ベースサイズとして保持
    }
}
