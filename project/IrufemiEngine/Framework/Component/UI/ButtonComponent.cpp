#include "ButtonComponent.h"
#include "../../GameObject.h"
#include "../../BaseScene.h"
#include "../TransformComponent.h"
#include "../Renderer/SpriteRendererComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "../../SceneTransition.h"

void ButtonComponent::OnRegisterProperties() {
    RegisterProperty("Load Scene Name", &onClickLoadScene_);
    // 0:Fade, 1:Dissolve, 2:Slide, 3:RadialBlur
    RegisterProperty("Transition Type(0-3)", &transitionType_);
    RegisterProperty("Transition Duration", &transitionDuration_);
    RegisterProperty("Normal Color", &normalColor_);
    RegisterProperty("Hover Color", &hoverColor_);
    RegisterProperty("Click Color", &clickColor_);
    RegisterProperty("Enable Hover Pulse", &enableHoverPulse_);
}

void ButtonComponent::Initialize() {
    if (gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
        sprite_ = gameObject_->GetComponent<SpriteRendererComponent>();
    }
}

bool ButtonComponent::CheckBounds(const Vector2& mousePos) {
    if (!transform_ || !sprite_) return false;
    
    // スプライトが実際に持つ大きさとスケールを考慮
    // anchor が 0.5 の場合は中心基準、0.0 の場合は左上基準など、エンジン仕様に合わせて判定
    // WP0 の SpriteRendererComponent は基本的に Transform の位置に Sprite を描画します。
    // 今回は簡易的に、位置(x,y)を中心に width/height の矩形と仮定するか、左上起点の矩形として判定します。
    
    Vector3 pos = transform_->worldPosition_;
    Vector3 scale = transform_->worldScale_;
    
    // スプライトの元サイズを取得
    auto* s = sprite_->GetSprite();
    if (!s) return false;
    
    Vector2 baseSize = s->GetSize();
    float width = baseSize.x * scale.x;
    float height = baseSize.y * scale.y;
    
    // アンカーが 0.5, 0.5 である前提で AABB を作成（SpriteRendererComponent のデフォルトは 0.5, 0.5）
    float left = pos.x - width * 0.5f;
    float right = pos.x + width * 0.5f;
    float top = pos.y - height * 0.5f;
    float bottom = pos.y + height * 0.5f;
    
    return (mousePos.x >= left && mousePos.x <= right &&
            mousePos.y >= top && mousePos.y <= bottom);
}

void ButtonComponent::Update() {
    if (!sprite_ || !gameObject_) return;
    
    auto scene = gameObject_->GetScene();
    if (!scene) return;
    auto engine = scene->GetEngine();
    if (!engine) return;
    
    auto input = engine->GetInputManager();
    Vector2 mousePos = input->GetMousePosition();
    
    isHovered_ = CheckBounds(mousePos);
    isClicked_ = false;

    // アニメーターの更新（1/60固定とするか deltaTime を取得するか。簡易的に1/60）
    // （※本来は GameApplication などの deltaTime が望ましいがUI用に一律でも動作する）
    animator_.Update(1.0f / 60.0f);

    if (isHovered_) {
        if (input->IsMouseButtonDown(Mouse::Button::Left)) {
            // 押下中
            sprite_->GetSprite()->SetColor(clickColor_);
        } else {
            // ホバー中
            Vector4 color = hoverColor_;
            if (enableHoverPulse_) {
                float animAlpha = animator_.GetPulseAlpha(0.7f, 0.3f, 5.0f);
                color.w *= animAlpha;
            }
            sprite_->GetSprite()->SetColor(color);
            
            // 離された瞬間（クリック完了）
            if (input->IsMouseButtonReleased(Mouse::Button::Left)) {
                isClicked_ = true;
                
                if (!onClickLoadScene_.empty()) {
                    // トランジション型変換
                    SceneTransition::Type type = SceneTransition::Type::Fade;
                    switch (transitionType_) {
                        case 0: type = SceneTransition::Type::Fade; break;
                        case 1: type = SceneTransition::Type::Dissolve; break;
                        case 2: type = SceneTransition::Type::Slide; break;
                        case 3: type = SceneTransition::Type::RadialBlur; break;
                    }
                    
                    // シーン遷移
                    engine->GetSceneManager()->LoadScene(onClickLoadScene_, type, transitionDuration_);
                }
            }
        }
    } else {
        // 通常状態
        animator_.Reset(); // ホバーが外れたらリセット
        sprite_->GetSprite()->SetColor(normalColor_);
    }
}
