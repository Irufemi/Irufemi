#pragma once
#include "../Component.h"
#include <string>
#include "Engine/Core/Math/Vector4.h"
#include "../../UIAnimator.h"

class TransformComponent;
class SpriteRendererComponent;

/**
 * @class ButtonComponent
 * @brief マウスのホバー・クリックを判定し、色変更やシーン遷移を行うUIコンポーネント
 */
class ButtonComponent : public Component {
public:
    ButtonComponent() = default;
    ~ButtonComponent() override = default;

    void Initialize() override;
    void Update() override;
    
    std::string GetComponentName() const override { return "ButtonComponent"; }
    void OnRegisterProperties() override;

    bool IsHovered() const { return isHovered_; }
    bool IsClicked() const { return isClicked_; }

private:
    bool CheckBounds(const struct Vector2& mousePos);
    
    Vector4 normalColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 hoverColor_  = {0.8f, 0.8f, 0.8f, 1.0f};
    Vector4 clickColor_{ 0.5f, 0.5f, 0.5f, 1.0f };

    bool enableHoverPulse_ = true; // ホバー時にサイン波で明滅するかどうか
    bool enableIdlePulse_ = true;  // 待機中（ホバーしていない時）も明滅するかどうか

    // 当たり判定のスケール調整（画像自体の余白などを省くため）
    Vector2 hitboxScale_{ 1.0f, 1.0f };

    // --- 内部状態 ---
    UIAnimator animator_;

    bool isHovered_ = false;
    bool isClicked_ = false; // クリックされた瞬間
    bool isPressedOnButton_ = false; // ボタン上で押下中かどうか

    TransformComponent* transform_ = nullptr;
    SpriteRendererComponent* sprite_ = nullptr;
};
