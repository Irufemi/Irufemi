#pragma once

#include "math/Vector2.h"
#include "math/Vector4.h"
#include <memory>
#include <vector>
#include <string>

// 前方宣言
class Camera;
class NumberText;
class Sprite;

// 時間の表示形式
enum class TimeFormat {
    HMS,        // 時間/分/秒 (例: 1:23:45)
    MS,         // 分/秒 (例: 23:45)
    S_DECIMAL,  // 秒.小数点以下2桁 (例: 45.67)
};

class TimeDisplay {
public:
    void Initialize(
        Camera* camera,
        TimeFormat format,
        const std::string& numberTexturePath,
        const Vector2& numberSize,
        const std::string& separatorTexturePath,
        const Vector2& separatorSize
    );

    void Update();
    void Draw(float timeInSeconds);

    // 表示位置を設定
    void SetPosition(const Vector2& position);

    // 全体の色を設定
    void SetColor(const Vector4& color);

private:
    // 桁ごとの数字スプライトを管理
    std::vector<std::unique_ptr<NumberText>> digits_;
    // 区切り文字スプライトを管理
    std::vector<std::unique_ptr<Sprite>> separators_;

    TimeFormat format_;
    Vector2 position_{ 0.0f, 0.0f };
    Vector2 numberSize_{ 0.0f, 0.0f };
    Vector2 separatorSize_{ 0.0f, 0.0f };
    Camera* camera_ = nullptr;
};