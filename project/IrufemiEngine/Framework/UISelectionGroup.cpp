#include "UISelectionGroup.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Renderer/Object2D/Sprite/Sprite.h"

UISelectionGroup::UISelectionGroup() {
    animator_.Reset();
}

void UISelectionGroup::AddItem(Sprite* sprite) {
    if (sprite) {
        items_.push_back(sprite);
    }
}

void UISelectionGroup::Reset() {
    isDecided_ = false;
    selectedIndex_ = 0;
    animator_.Reset();
}

void UISelectionGroup::Update(InputManager* input) {
    // 1フレームの時間を 1.0f / 60.0f と仮定
    animator_.Update(1.0f / 60.0f);

    if (isDecided_ || items_.empty() || !input) return;

    bool isMenuChanged = false;

    // 上キー (W または UP)
    if (input->IsKeyPressedDIK(0x11 /* W */) || input->IsKeyPressedDIK(0xC8 /* UP */)) {
        selectedIndex_--;
        if (selectedIndex_ < 0) {
            selectedIndex_ = static_cast<int>(items_.size()) - 1;
        }
        isMenuChanged = true;
    }
    // 下キー (S または DOWN)
    if (input->IsKeyPressedDIK(0x1F /* S */) || input->IsKeyPressedDIK(0xD0 /* DOWN */)) {
        selectedIndex_++;
        if (selectedIndex_ >= static_cast<int>(items_.size())) {
            selectedIndex_ = 0;
        }
        isMenuChanged = true;
    }

    if (isMenuChanged) {
        animator_.Reset(); // 項目切り替え時にアニメーションをリセット
    }

    // 決定キー (Space または Enter または テンキーのEnter)
    if (input->IsKeyPressedDIK(0x39 /* Space */) || 
        input->IsKeyPressedDIK(0x1C /* Enter */) || 
        input->IsKeyPressedDIK(0x9C /* Numpad Enter */)) {
        isDecided_ = true;
    }
}

void UISelectionGroup::Draw() {
    if (items_.empty()) return;

    // サイン波による明滅（0.7〜1.0など）をベースカラーのアルファに乗算する
    float animAlpha = animator_.GetPulseAlpha(0.7f, 0.3f, 5.0f);
    Vector4 currentActiveColor = activeBaseColor_;
    currentActiveColor.w *= animAlpha;

    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        Sprite* sprite = items_[i];
        if (!sprite) continue;

        if (i == selectedIndex_) {
            sprite->SetColor(currentActiveColor);
        } else {
            sprite->SetColor(inactiveColor_);
        }
        
        sprite->Draw();
    }
}
