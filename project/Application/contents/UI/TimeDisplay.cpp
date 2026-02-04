#include "TimeDisplay.h"
#include "contents/UI/NumberText.h"
#include "2D/Sprite.h"
#include <cmath>

void TimeDisplay::Initialize(
    Camera* camera,
    TimeFormat format,
    const std::string& numberTextureBasePath,
    const std::string& numberTextureFileExtension,
    const Vector2& numberSize,
    const std::string& separatorTexturePath,
    const Vector2& separatorSize) {

    camera_ = camera;
    format_ = format;
    numberSize_ = numberSize;
    separatorSize_ = separatorSize;

    int numDigits = 0;
    int numSeparators = 0;

    switch (format_) {
    case TimeFormat::HMS:
        numDigits = 6; // hh:mm:ss
        numSeparators = 2;
        break;
    case TimeFormat::MS:
        numDigits = 4; // mm:ss
        numSeparators = 1;
        break;
    case TimeFormat::S_DECIMAL:
        numDigits = 4; // ss.ss
        numSeparators = 1;
        break;
    }

    // 数字用のNumberTextを生成
    digits_.resize(numDigits);
    for (int i = 0; i < numDigits; ++i) {
        digits_[i] = std::make_unique<NumberText>();
        digits_[i]->Initialize(camera_, numberTextureBasePath, numberTextureFileExtension, numberSize_.x, numberSize_.y);
    }

    // 区切り文字用のSpriteを生成
    separators_.resize(numSeparators);
    for (int i = 0; i < numSeparators; ++i) {
        separators_[i] = std::make_unique<Sprite>();
        // 区切り文字は個別のテクスチャとして読み込む
        separators_[i]->Initialize(camera_, separatorTexturePath);
        separators_[i]->SetSize(separatorSize_.x, separatorSize_.y);
        separators_[i]->SetAnchor(0.0f, 0.0f); // 左上基準
    }
}

void TimeDisplay::Update() {
    // 各スプライトのUpdateを呼び出す
    for (auto& digit : digits_) {
        // NumberTextのDraw内でUpdateが呼ばれるため、ここでは不要
    }
    for (auto& separator : separators_) {
        separator->Update();
    }
}

void TimeDisplay::Draw(float timeInSeconds) {
    int h = static_cast<int>(timeInSeconds / 3600);
    int m = static_cast<int>(fmod(timeInSeconds, 3600) / 60);
    int s = static_cast<int>(fmod(timeInSeconds, 60));
    int ms = static_cast<int>(fmod(timeInSeconds, 1.0f) * 100);

    float currentPosX = position_.x;

    auto drawTwoDigits = [&](int value, int digitIndex1, int digitIndex2) {
        digits_[digitIndex1]->SetPosition({ currentPosX, position_.y });
        digits_[digitIndex1]->Draw(value / 10);
        currentPosX += numberSize_.x;

        digits_[digitIndex2]->SetPosition({ currentPosX, position_.y });
        digits_[digitIndex2]->Draw(value % 10);
        currentPosX += numberSize_.x;
    };

    auto drawSeparator = [&](int separatorIndex) {
        if (separatorIndex < separators_.size()) {
            separators_[separatorIndex]->SetPosition(currentPosX, position_.y);
            separators_[separatorIndex]->Update(); // 位置変更を反映
            separators_[separatorIndex]->Draw();
            currentPosX += separatorSize_.x;
        }
    };

    switch (format_) {
    case TimeFormat::HMS:
        drawTwoDigits(h, 0, 1);
        drawSeparator(0);
        drawTwoDigits(m, 2, 3);
        drawSeparator(1);
        drawTwoDigits(s, 4, 5);
        break;

    case TimeFormat::MS:
        drawTwoDigits(m, 0, 1);
        drawSeparator(0);
        drawTwoDigits(s, 2, 3);
        break;

    case TimeFormat::S_DECIMAL:
        drawTwoDigits(s, 0, 1);
        drawSeparator(0);
        drawTwoDigits(ms, 2, 3);
        break;
    }
}

void TimeDisplay::SetPosition(const Vector2& position) {
    position_ = position;
}

void TimeDisplay::SetColor(const Vector4& color) {
    for (auto& digit : digits_) {
        digit->SetColor(color);
    }
    for (auto& separator : separators_) {
        separator->SetColor(color);
    }
}