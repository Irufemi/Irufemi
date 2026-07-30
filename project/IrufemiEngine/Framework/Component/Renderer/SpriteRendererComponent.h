#pragma once
#include "../Component.h"
#include "Renderer/Object/2D/Sprite/Sprite.h"
#include <memory>
#include <string>
#include "Engine/Core/Math/Vector4.h"

class TransformComponent;

/**
 * @class SpriteRendererComponent
 * @brief 2Dスプライト描画用コンポーネント
 * @details GameObjectにアタッチして2D画像の描画とUI操作を提供します
 */
class SpriteRendererComponent : public Component {
public:
    SpriteRendererComponent();
    virtual ~SpriteRendererComponent();

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
    IRenderable* GetRenderable() override { return reinterpret_cast<IRenderable*>(sprite_.get()); }
#ifdef EditorMode
    friend class SpriteRendererComponentEditor;
#endif
    /**
     * @brief Texture を設定する。
     * @param[in] texturePath 設定する Texture の値
     */
    void SetTexture(const std::string& texturePath);
    /**
     * @brief Sprite を取得する。
     * @return 取得された Sprite
     */
    Sprite* GetSprite() const { return sprite_.get(); }

    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override { return "SpriteRendererComponent"; }
    /**
     * @brief Serialize を実行する。
     */
    nlohmann::json Serialize() override;
    /**
     * @brief Deserialize を実行する。
     */
    void Deserialize(const nlohmann::json& j) override;

private:
    std::unique_ptr<Sprite> sprite_;
    TransformComponent* transform_ = nullptr;
    
    std::string texturePath_ = "";
    bool isTopMost_ = false;
    bool isFlipX_ = false;
    bool isFlipY_ = false;
    float anchor_[2] = { 0.5f, 0.5f };
    float size_[2] = { 640.0f, 360.0f };
    Irufemi::Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
};
