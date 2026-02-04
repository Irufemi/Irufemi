#include "NumberText.h"
#include "Application/camera/Camera.h"
#include <string>

void NumberText::Initialize(Camera* camera, const std::string& textureBasePath, const std::string& fileExtension, float numberWidth, float numberHeight) {
    camera_ = camera;
    spriteSize_ = { numberWidth, numberHeight };

    // 0から9までのスプライトを生成・初期化
    for (int i = 0; i < 10; ++i) {
        numberSprites_[i] = std::make_unique<Sprite>();
        // "basePath" + "i" + ".png" のようなパスを生成
        std::string texturePath = textureBasePath + std::to_string(i) + fileExtension;
        numberSprites_[i]->Initialize(camera_, texturePath);
        numberSprites_[i]->SetSize(spriteSize_.x, spriteSize_.y);
        numberSprites_[i]->SetAnchor(0.0f, 0.0f); // 左上基準
    }
}

void NumberText::Draw(int number) {
    // 1桁の数字のみを扱う
    int digit = number % 10;

    if (digit >= 0 && digit < 10) {
        // 対応する数字のスプライトを取得
        Sprite* sprite = numberSprites_[digit].get();
        if (sprite) {
            // 描画位置を設定
            sprite->SetPosition(position_.x, position_.y);
            // 設定を反映させるためにUpdateを呼び出す
            sprite->Update();
            // スプライトを描画
            sprite->Draw();
        }
    }
}

void NumberText::SetPosition(const Vector2& position) {
    position_ = position;
}

void NumberText::SetSize(float width, float height) {
    spriteSize_ = { width, height };
    for (auto& sprite : numberSprites_) {
        if (sprite) {
            sprite->SetSize(width, height);
        }
    }
}

void NumberText::SetColor(const Vector4& color) {
    for (auto& sprite : numberSprites_) {
        if (sprite) {
            sprite->SetColor(color);
        }
    }
}