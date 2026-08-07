#pragma once
#include "../Component.h"
#include "Renderer/Object/2D/Text/Text.h"
#include <memory>
#include <string>
#include "Engine/Core/Math/Vector4.h"

class TransformComponent;

/**
 * @class TextRendererComponent
 * @brief 2Dテキスト描画用コンポーネント (MSDF対応)
 */
class TextRendererComponent : public Component {
public:
    TextRendererComponent();
    virtual ~TextRendererComponent();

    /**
     * @brief Initialize を実行する。
     */
    void Initialize() override;
    /**
     * @brief Update を実行する。
     */
    void Update() override;
    /**
     * @brief Draw を実行する。
     */
    void Draw() override;
    
    /**
     * @brief CanUpdateInEditMode かどうかを判定する。
     * @return 判定結果 (true/false)
     */
    bool CanUpdateInEditMode() const override { return true; }
    
    // エディタのRaycast用
    /**
     * @brief Raycast を実行する。
     */
    bool Raycast(const Irufemi::Ray& ray, float& outDistance) const override;
    
    /**
     * @brief Renderable を取得する。
     * @return 取得された Renderable
     */
    IRenderable* GetRenderable() override { return reinterpret_cast<IRenderable*>(textObj_.get()); }
    /**
     * @brief DrawOutlineMask を実行する。
     */
    void DrawOutlineMask() override {
        if (textObj_) {
            textObj_->DrawOutlineMask();
        }
    }

    // 文字列の設定
    /**
     * @brief Text を設定する。
     * @param[in] text 設定する Text の値
     */
    void SetText(const std::wstring& text);
    /**
     * @brief Text を取得する。
     * @return 取得された Text
     */
    std::wstring GetText() const { return text_; }

    // フォントの変更
    /**
     * @brief FontId を設定する。
     * @param[in] fontId 設定する FontId の値
     */
    void SetFontId(const std::string& fontId);
    /**
     * @brief FontId を取得する。
     * @return 取得された FontId
     */
    std::string GetFontId() const { return fontId_; }

    // ベーススケール（文字サイズ）
    /**
     * @brief BaseScale を設定する。
     * @param[in] baseScale 設定する BaseScale の値
     */
    void SetBaseScale(float baseScale);
    /**
     * @brief BaseScale を取得する。
     * @return 取得された BaseScale
     */
    float GetBaseScale() const { return baseScale_; }

    // アライメント
    /**
     * @brief Alignment を設定する。
     * @param[in] align 設定する Alignment の値
     */
    void SetAlignment(TextAlignment align);
    /**
     * @brief Alignment を取得する。
     * @return 取得された Alignment
     */
    TextAlignment GetAlignment() const { return alignment_; }

    // 文字色
    /**
     * @brief Color を設定する。
     * @param[in] color 設定する Color の値
     */
    void SetColor(const Irufemi::Vector4& color);
    /**
     * @brief Color を取得する。
     * @return 取得された Color
     */
    Irufemi::Vector4 GetColor() const { return color_; }
    
    // UIとして最前面に描画するか
    /**
     * @brief TopMost を設定する。
     * @param[in] isTopMost 設定する TopMost の値
     */
    void SetTopMost(bool isTopMost);
    /**
     * @brief IsTopMost かどうかを判定する。
     * @return 判定結果 (true/false)
     */
    bool IsTopMost() const { return isTopMost_; }
    
    // バウンディングボックス取得（ローカル座標系）
    /**
     * @brief LocalBoundsMin を取得する。
     * @return 取得された LocalBoundsMin
     */
    Irufemi::Vector2 GetLocalBoundsMin() const { return textObj_ ? textObj_->GetLocalBoundsMin() : Irufemi::Vector2{0.0f, 0.0f}; }
    /**
     * @brief LocalBoundsMax を取得する。
     * @return 取得された LocalBoundsMax
     */
    Irufemi::Vector2 GetLocalBoundsMax() const { return textObj_ ? textObj_->GetLocalBoundsMax() : Irufemi::Vector2{0.0f, 0.0f}; }

    /**
     * @brief TextObject を取得する。
     * @return 取得された TextObject
     */
    Text* GetTextObject() const { return textObj_.get(); }

    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override { return "TextRendererComponent"; }
    /**
     * @brief OnRegisterProperties を実行する。
     */
    void OnRegisterProperties() override;

private:
    std::unique_ptr<Text> textObj_;
    std::wstring text_ = L"Text";
    std::string textU8_ = "Text"; // For Reflection
    std::string fontId_ = "MainFont";
    float baseScale_ = 64.0f;
    Irufemi::Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    TextAlignment alignment_ = TextAlignment::Left;
    int alignmentInt_ = 0; // For Reflection (0:Left, 1:Center, 2:Right)
    bool isTopMost_ = false;
};
