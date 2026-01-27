#include "NumberText.h"
#include "Application/camera/Camera.h"
#include <string>

void NumberText::Initialize(Camera* camera, const std::string& texturePath, float numberWidth, float numberHeight) {
    camera_ = camera;
    texturePath_ = texturePath;
    numberWidth_ = numberWidth;
    numberHeight_ = numberHeight;
    spriteSize_ = { numberWidth, numberHeight };

    // スプライトを1つだけ生成
    numberSprite_ = std::make_unique<Sprite>();
    numberSprite_->Initialize(camera_, texturePath_);
    numberSprite_->SetSize(spriteSize_.x, spriteSize_.y);
    numberSprite_->SetAnchor(0.0f, 0.0f); // 左上基準
}

void NumberText::Draw(int number) {
    // 1桁の数字のみを扱う
    int digit = number % 10;

    // 描画位置を設定
    numberSprite_->SetPosition(position_.x, position_.y);

    // テクスチャの切り出し範囲を設定
    int texX = static_cast<int>(digit * numberWidth_);
    int texY = 0;
    numberSprite_->SetTextureRectPixels(texX, texY, static_cast<int>(numberWidth_), static_cast<int>(numberHeight_));

    // 設定を反映させるためにUpdateを呼び出す
    numberSprite_->Update();

    // スプライトを描画
    numberSprite_->Draw();
}

void NumberText::SetPosition(const Vector2& position) {
    position_ = position;
}

void NumberText::SetSize(float width, float height) {
    spriteSize_ = { width, height };
    if (numberSprite_) {
        numberSprite_->SetSize(width, height);
    }
}

void NumberText::SetColor(const Vector4& color) {
    if (numberSprite_) {
        numberSprite_->SetColor(color);
    }
}