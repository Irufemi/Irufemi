#pragma once

#include "../Primitive/PrimitiveObjects3DClass.h"
#include "../../../Engine/Core/Math/Vector2.h"
#include "../../../Engine/Core/Type/PrimitiveType.h"
#include "../../../Engine/Graphics/Pipeline/PSOManager.h"
#include "../../../Resource/Texture/TextureManager.h"
#include <string>

/**
 * @class PrimitiveEffect
 * @brief 汎用エフェクトの基盤クラス
 * @details PrimitiveObjects3DClass を内包し、以下のエフェクト特有の機能を提供します。
 * - UVスクロール（時間経過によるテクスチャ移動）
 * - UV反転（ScaleのON/OFF）
 * - discard制御（特定のアルファ値以下を描画しない処理の設定）
 * - Cullingモードの動的切り替え
 * - 特別な形状パラメータ（Cylinderの上下半径分離など）の再生成機能
 */
class PrimitiveEffect {
public:
    PrimitiveEffect() = default;
    ~PrimitiveEffect() = default;

    /**
     * @brief 初期化
     * @param camera 描画に使用するカメラ
     * @param type 形状
     * @param texturePath 使用するテクスチャ
     */
    static void SetTextureManager(TextureManager* texManager) { textureManager_ = texManager; }

    void Initialize(Camera* camera, PrimitiveType type, const std::string& texturePath = "resources/uvChecker.png");

    /**
     * @brief 更新処理（UVスクロールのアニメーションなどを進める）
     * @param deltaTime デルタタイム
     */
    void Update(float deltaTime);

    /**
     * @brief 描画処理
     * @details CullMode を一時的に本クラスの設定に合わせて描画し、その後元に戻す等の処理をします（必要ならパイプライン経由対応）
     */
    void Draw();

    /**
     * @brief デバッグ・表示用UIの描画
     */
    void Debug(const char* label = "Primitive Effect");

    // --- アクセサ ---
    PrimitiveObjects3DClass& GetPrimitive() { return primitive_; }
    void SetTexture(const std::string& path) { primitive_.SetTexture(path); }

    void SetUVScrollSpeed(const Vector2& speed) { uvScrollSpeed_ = speed; }
    void SetUVScale(const Vector2& scale) { uvScale_ = scale; }
    void SetAlphaReference(float alphaRef) { primitive_.GetMaterial().alphaReference = alphaRef; }
    
    // --- カスタム形状パラメータ設定 ---
    void SetCylinderParams(float bottomRadius, float topRadius, float height, uint32_t segments, bool hasTop, bool hasBottom, bool centered);

public: // エフェクト用の公開設定（外部から直接いじれるようにpublic）
    Vector2 uvScrollOffset_{ 0.0f, 0.0f };
    Vector2 uvScrollSpeed_{ 0.0f, 0.0f };
    Vector2 uvScale_{ 1.0f, 1.0f };
    
    PSOManager::CullMode cullMode_ = PSOManager::CullMode::None;
    int selectedTextureIndex_ = -1;

private:
    PrimitiveObjects3DClass primitive_;
    static TextureManager* textureManager_;

    // Cylinder再生成用キャッシュ
    bool isCylinderMode_ = false;
    float cylBottomRadius_ = 1.0f;
    float cylTopRadius_ = 1.0f;
    float cylHeight_ = 2.0f;
    uint32_t cylSegments_ = 32;
    bool cylHasTop_ = false;
    bool cylHasBottom_ = false;
    bool cylCentered_ = false;
};
