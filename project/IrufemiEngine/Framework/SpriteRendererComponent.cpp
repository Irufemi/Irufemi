#include "SpriteRendererComponent.h"
#include "GameObject.h"
#include "TransformComponent.h"

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
        sprite_->SetPosition(transform_->position_.x, transform_->position_.y, transform_->position_.z);
        // Spriteの回転はZ軸のみ
        sprite_->SetRotation(transform_->rotation_.z);
        
        // TransformのScaleは、SpriteのBaseサイズに対するスケーリングとして扱う
        sprite_->SetSize(size_[0] * transform_->scale_.x, size_[1] * transform_->scale_.y);
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
        
        // 簡易的に直接SpriteのデバッグUIを呼び出す
        // Sprite::Debug() にはテクスチャ選択などが揃っているため便利
        if (sprite_) {
            ImGui::Text("Sprite Internal Properties");
            ImGui::Separator();
            // Windowを開かずに現在のコンテキストに描画するため、
            // 少し特殊だが今回はCheckboxなどを自前で用意する
            
            bool needUpdate = false;
            
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
                // サイズ変更。実描画サイズは Transform.Scale が乗算される
            }
            
            if (ImGui::ColorEdit4("Color", &color_.x)) {
                sprite_->SetColor(color_);
            }
            
            ImGui::Separator();
            if (ImGui::Button("Open Detailed Sprite Editor")) {
                ImGui::OpenPopup("SpriteDetailedPopup");
            }
            
            if (ImGui::BeginPopup("SpriteDetailedPopup")) {
                // SpriteクラスのDebugを直接呼ぶとWindowが生成されてしまうため、
                // Popupとして利用するのは難しいが、この中で設定を変えられる
                ImGui::Text("Detailed settings are managed by internal Sprite class.");
                sprite_->Debug("Component Sprite");
                ImGui::EndPopup();
            }
        }

        ImGui::TreePop();
    }
#endif
}
