#include "Framework/Component/UI/ButtonComponent.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/SpriteRendererComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Platform/Input/InputManager.h"
#include "Framework/Scene/SceneTransition.h"
#include "Renderer/Camera/CameraManager.h"
#include "Renderer/Camera/Camera.h"

void ButtonComponent::OnRegisterProperties() {
    RegisterProperty("Normal Color", &normalColor_);
    RegisterProperty("Hover Color", &hoverColor_);
    RegisterProperty("Click Color", &clickColor_);
    RegisterProperty("Enable Hover Pulse", &enableHoverPulse_);
    RegisterProperty("Enable Idle Pulse", &enableIdlePulse_);
    RegisterProperty("Hitbox Scale", &hitboxScale_);
}

void ButtonComponent::Initialize() {
    if (gameObject_) {
        sprite_ = gameObject_->GetComponent<SpriteRendererComponent>();
    }
}

bool ButtonComponent::CheckBounds(const Irufemi::Vector2& mousePos) {
    if (!GetTransform() || !sprite_) {
        return false;
    }

    Irufemi::Vector3 pos = GetTransform()->GetWorldPosition();
    Irufemi::Vector3 scale = GetTransform()->GetWorldScale();

    auto* s = sprite_->GetSprite();
    if (!s) {
        return false;
    }

    // スプライトのアンカーとサイズを取得
    Irufemi::Vector2 anchor = s->GetAnchor();
    Irufemi::Vector2 baseSize = s->GetSize();

    // Hitbox Scale を加味した幅・高さを算出
    // （※sprite_->GetSize() は既に Irufemi::Transform の Scale が適用された描画上のサイズを返すため、
    //  ここでは scale.x/y を二重に掛けないようにする）
    float width = baseSize.x * hitboxScale_.x;
    float height = baseSize.y * hitboxScale_.y;

    // アンカー位置を加味して当たり判定矩形を計算
    // （例えば anchor が 0.5 の場合、pos が中心になる）
    float left = pos.x - width * anchor.x;
    float right = pos.x + width * (1.0f - anchor.x);
    float top = pos.y - height * anchor.y;
    float bottom = pos.y + height * (1.0f - anchor.y);

    return (mousePos.x >= left && mousePos.x <= right && mousePos.y >= top && mousePos.y <= bottom);
}

void ButtonComponent::Update() {
    if (!sprite_ || !gameObject_) {
        return;
    }

    auto scene = gameObject_->GetScene();
    if (!scene) {
        return;
    }
    auto engine = scene->GetEngine();
    if (!engine) {
        return;
    }

    auto input = engine->GetInputManager();
    auto cameraManager = engine->GetCameraManager();
    if (!input || !cameraManager || !cameraManager->GetActiveCamera()) {
        return;
    }

    Irufemi::Vector2 mousePos = input->GetMousePosition();
    Irufemi::Vector2 uiPos = cameraManager->GetActiveCamera()->ScreenToUIPosition(mousePos);

    isHovered_ = CheckBounds(uiPos);
    isClicked_ = false;

    // アニメーターの更新（1/60固定とするか deltaTime を取得するか。簡易的に1/60）
    animator_.Update(1.0f / 60.0f);

    if (isHovered_) {
        // ホバーした瞬間に押下されたらフラグを立てる
        if (input->IsMouseButtonPressed(Mouse::Button::Left)) {
            isPressedOnButton_ = true;
        }

        if (input->IsMouseButtonDown(Mouse::Button::Left)) {
            // 押下中（ボタン上で押下開始した場合のみ色を変える）
            if (isPressedOnButton_) {
                sprite_->GetSprite()->SetColor(clickColor_);
            } else {
                sprite_->GetSprite()->SetColor(normalColor_);
            }
        } else {
            // ホバー中
            Irufemi::Vector4 color = hoverColor_;
            if (enableHoverPulse_) {
                float animAlpha = animator_.GetPulseAlpha(0.7f, 0.3f, 5.0f);
                color.w *= animAlpha;
            }
            sprite_->GetSprite()->SetColor(color);

            // 離された瞬間（クリック完了）
            if (input->IsMouseButtonReleased(Mouse::Button::Left) && isPressedOnButton_) {
                isClicked_ = true;
            }
        }
    } else {
        // 通常状態（待機中）
        Irufemi::Vector4 color = normalColor_;
        if (enableIdlePulse_) {
            // PromptControllerと同じパルスアニメーション（ベース0.6、振幅0.4、速度3.0）
            float animAlpha = animator_.GetPulseAlpha(0.6f, 0.4f, 3.0f);
            color.w *= animAlpha;
        } else {
            // アニメーション無効時はリセットしておく
            animator_.Reset();
        }
        sprite_->GetSprite()->SetColor(color);
    }

    // どこかでマウスが離されたらフラグをリセットする
    if (input->IsMouseButtonReleased(Mouse::Button::Left)) {
        isPressedOnButton_ = false;
    }
}
