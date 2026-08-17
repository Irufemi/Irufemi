#pragma once
#include "Framework/Component/Component.h"
#include "Renderer/Object/2D/Primitive/Primitive2DObject.h"
#include "Core/Type/Primitive2DType.h"
#include <memory>
#include <string>
#include "Core/Math/Vector4.h"
#include "Core/Math/Vector2.h"

class TransformComponent;

/**
 * @class Primitive2DRendererComponent
 * @brief 2Dプリミティブ描画用コンポーネント
 * @details GameObjectにアタッチして汎用的な2Dプリミティブ（四角、円、線など）の描画とUI操作を提供します
 */
class Primitive2DRendererComponent : public Component {
public:
    Primitive2DRendererComponent();
    virtual ~Primitive2DRendererComponent();

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
    
    /**
     * @brief Renderable を取得する。
     * @return 取得された Renderable
     */
    IRenderable* GetRenderable() override { return reinterpret_cast<IRenderable*>(primitive_.get()); }

#ifdef EditorMode
    friend class Primitive2DRendererComponentEditor;
#endif

    // プロパティ操作
    /**
     * @brief Shape を設定する。
     * @param[in] type 設定する Shape の値
     */
    void SetShape(Irufemi::Primitive2DType type);
    /**
     * @brief Color を設定する。
     * @param[in] color 設定する Color の値
     */
    void SetColor(const Irufemi::Vector4& color);
    /**
     * @brief Texture を設定する。
     * @param[in] texturePath 設定する Texture の値
     */
    void SetTexture(const std::string& texturePath);
    /**
     * @brief Pivot を設定する。
     * @param[in] pivot 設定する Pivot の値
     */
    void SetPivot(const Irufemi::Vector2& pivot);
    /**
     * @brief Size を設定する。
     * @param[in] size 設定する Size の値
     */
    void SetSize(const Irufemi::Vector2& size);
    /**
     * @brief Thickness を設定する。
     * @param[in] thickness 設定する Thickness の値
     */
    void SetThickness(float thickness);
    /**
     * @brief Subdivision を設定する。
     * @param[in] subdivision 設定する Subdivision の値
     */
    void SetSubdivision(int subdivision);
    /**
     * @brief TopMost を設定する。
     * @param[in] isTopMost 設定する TopMost の値
     */
    void SetTopMost(bool isTopMost);

    // プロパティ取得（Editor用など）
    /**
     * @brief Shape を取得する。
     * @return 取得された Shape
     */
    Irufemi::Primitive2DType GetShape() const { return static_cast<Irufemi::Primitive2DType>(currentTypeIndex_); }
    /**
     * @brief Color を取得する。
     * @return 取得された Color
     */
    const Irufemi::Vector4& GetColor() const { return color_; }
    /**
     * @brief Texture を取得する。
     * @return 取得された Texture
     */
    const std::string& GetTexture() const { return texturePath_; }
    /**
     * @brief Pivot を取得する。
     * @return 取得された Pivot
     */
    const Irufemi::Vector2& GetPivot() const { return pivot_; }
    /**
     * @brief Size を取得する。
     * @return 取得された Size
     */
    const Irufemi::Vector2& GetSize() const { return size_; }
    /**
     * @brief Thickness を取得する。
     * @return 取得された Thickness
     */
    float GetThickness() const { return thickness_; }
    /**
     * @brief Subdivision を取得する。
     * @return 取得された Subdivision
     */
    int GetSubdivision() const { return subdivision_; }
    /**
     * @brief IsTopMost かどうかを判定する。
     * @return 判定結果 (true/false)
     */
    bool IsTopMost() const { return isTopMost_; }

    /**
     * @brief Primitive を取得する。
     * @return 取得された Primitive
     */
    Primitive2DObject* GetPrimitive() const { return primitive_.get(); }
    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override { return "Primitive2DRendererComponent"; }
    
    /**
     * @brief Serialize を実行する。
     */
    nlohmann::json Serialize() override;
    /**
     * @brief Deserialize を実行する。
     */
    void Deserialize(const nlohmann::json& j) override;

private:
    std::unique_ptr<Primitive2DObject> primitive_;
    int currentTypeIndex_ = 0; // デフォルトは Rect
    std::string texturePath_ = "";
    bool isTopMost_ = false;
    
    // 描画パラメータのバックアップ（Primitive2DObject生成前やSerialize用）
    Irufemi::Vector2 size_ = { 100.0f, 100.0f };
    Irufemi::Vector2 pivot_ = { 0.5f, 0.5f };
    Irufemi::Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    float thickness_ = 2.0f;
    int subdivision_ = 32;
};
