#pragma once

#include "../../../System/Core/IRenderable.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "Engine/Core/Type/Primitive2DType.h"
#include "Renderer/System/Core/Object2DResource.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

/**
 * @class Primitive2DObject
 * @brief 汎用的な2Dプリミティブ（四角形、円、線など）を管理・描画するクラス
 * @details SizeとScaleを分離し、Pivot（原点）をベースにメッシュを再構築する設計。
 */
class Primitive2DObject : public IRenderable {
public:
    Primitive2DObject() = default;
    ~Primitive2DObject() = default;

    /**
     * @brief 初期化処理
     * @param[in] type 初期形状タイプ
     * @param[in] textureName 使用するテクスチャのパス（デフォルトは白テクスチャ推奨）
     */
    void Initialize(Primitive2DType type = Primitive2DType::Rect, const std::string& textureName = "resources/whiteTexture.png");

    /**
     * @brief 更新処理
     */
    void Update();

    /**
     * @brief 描画コマンド発行前の同期処理
     */
    void SyncBeforeDraw() override;

    /**
     * @brief 描画処理
     */
    void Draw() override;

    /**
     * @brief デバッグ・編集用UIを表示する
     * @param[in] label UIウィンドウおよび識別用のラベル
     */
    void Debug(const char* label = "Primitive 2D Object");

    // --- アクセサ・ゲッター ---
    Object2DResource* GetD3D12Resource() { return resource_.get(); }
    Primitive2DType GetShape() const { return type_; }
    const Vector2& GetSize() const { return size_; }
    const Vector2& GetPivot() const { return pivot_; }
    const Vector3& GetPosition() const { return resource_->transform_.translate; }
    const Vector3& GetRotation() const { return resource_->transform_.rotate; }
    const Vector3& GetScale() const { return resource_->transform_.scale; }
    const Vector4& GetColor() const { return resource_->GetMaterialData()->color; }

    // --- プロパティのセッター ---

    /**
     * @brief 形状を変更し、必要に応じてメッシュを再構築する
     */
    void SetShape(Primitive2DType type);

    /**
     * @brief 基準サイズを設定する（ピクセル単位などを想定）
     */
    void SetSize(const Vector2& size);

    /**
     * @brief ピボット（アンカー）を設定する（0.0～1.0）
     * @details 左上={0,0}, 中心={0.5,0.5}, 右下={1,1} など
     */
    void SetPivot(const Vector2& pivot);

    /**
     * @brief 位置（World Transform）を設定する
     */
    void SetPosition(const Vector3& position);
    void SetPosition(const Vector2& position) { SetPosition({position.x, position.y, 0.0f}); }

    /**
     * @brief 回転（Z軸のみ想定）を設定する
     */
    void SetRotationZ(float rad);

    /**
     * @brief スケール（倍率）を設定する
     */
    void SetScale(const Vector3& scale);
    void SetScale(const Vector2& scale) { SetScale({scale.x, scale.y, 1.0f}); }

    /**
     * @brief ベースカラーを設定する
     */
    void SetColor(const Vector4& color);

    /**
     * @brief テクスチャを設定する
     */
    void SetTexture(const std::string& textureName);

    /**
     * @brief 最前面描画（UIなど）のフラグを設定する
     */
    void SetTopMost(bool isTopMost) { isTopMost_ = isTopMost; }

    /**
     * @brief リング形状などでの「太さ（線幅など）」を設定する（ピクセル指定等）
     */
    void SetThickness(float thickness);

    // --- 各種マネージャの静的設定 ---
    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    static void SetEngine(class IrufemiEngine* engine) { engine_ = engine; }

private:
    /**
     * @brief 現在の type_, size_, pivot_ に基づいてメッシュ（頂点・インデックス）を再構築する
     */
    void RebuildMesh();

    // 内部メッシュ構築用ヘルパー
    void BuildRect();
    void BuildTriangle();
    void BuildCircle(uint32_t subdiv);
    void BuildRing(uint32_t subdiv);
    void BuildLine();

private:
    std::unique_ptr<Object2DResource> resource_ = nullptr;

    Primitive2DType type_ = Primitive2DType::Rect;
    Vector2 size_ = { 100.0f, 100.0f };
    Vector2 pivot_ = { 0.5f, 0.5f };
    float thickness_ = 10.0f;     //!< RingやLineでの太さ
    uint32_t subdivision_ = 64;   //!< 円系の分割数

    bool isTopMost_ = false;      //!< UI用など、最前面描画フラグ
    bool isMeshDirty_ = true;     //!< メッシュ再構築フラグ
    int selectedTextureIndex_ = 0; //!< デバッグUI用

    // 静的マネージャ参照
    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;
    static class IrufemiEngine* engine_;
};
