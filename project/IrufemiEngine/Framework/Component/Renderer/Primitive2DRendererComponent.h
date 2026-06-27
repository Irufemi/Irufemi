#pragma once
#include "../Component.h"
#include "Renderer/Object/2D/Primitive/Primitive2DObject.h"
#include "Engine/Core/Type/Primitive2DType.h"
#include <memory>
#include <string>
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Vector2.h"

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

    void Initialize() override;
    void Update() override;
    void Draw() override;
    
    bool CanUpdateInEditMode() const override { return true; }
    
    IRenderable* GetRenderable() override { return reinterpret_cast<IRenderable*>(primitive_.get()); }

#ifdef EditorMode
    friend class Primitive2DRendererComponentEditor;
#endif

    // プロパティ操作
    void SetShape(Primitive2DType type);
    void SetColor(const Vector4& color);
    void SetTexture(const std::string& texturePath);
    void SetPivot(const Vector2& pivot);
    void SetSize(const Vector2& size);
    void SetThickness(float thickness);
    void SetSubdivision(int subdivision);
    void SetTopMost(bool isTopMost);

    Primitive2DObject* GetPrimitive() const { return primitive_.get(); }
    std::string GetComponentName() const override { return "Primitive2DRendererComponent"; }
    
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

private:
    std::unique_ptr<Primitive2DObject> primitive_;
    TransformComponent* transform_ = nullptr;
    
    int currentTypeIndex_ = 0; // デフォルトは Rect
    std::string texturePath_ = "";
    bool isTopMost_ = false;
    
    // 描画パラメータのバックアップ（Primitive2DObject生成前やSerialize用）
    Vector2 size_ = { 100.0f, 100.0f };
    Vector2 pivot_ = { 0.5f, 0.5f };
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    float thickness_ = 2.0f;
    int subdivision_ = 32;
};
