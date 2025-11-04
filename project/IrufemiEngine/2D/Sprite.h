#pragma once

#include <d3d12.h>
#include <vector>
#include <cstdint>
#include "../source/D3D12ResourceUtil.h"
#include "../camera/Camera.h"
#include "../math/Vector2.h" 
#include <wrl.h>
#include <memory>

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

class Sprite {
private:

    std::unique_ptr<D3D12ResourceUtil> resource_ = nullptr;

    bool isRotateY_ = true;

    int selectedTextureIndex_ = 0;

    Camera* camera_ = nullptr;

    static TextureManager* textureManager_;

    static DrawManager* drawManager_;

    static DebugUI* ui_;

    // サイズとアンカー
    Vector2 size_{ 640.0f, 360.0f };   // 既存の見た目互換のため初期値を640x360に
    Vector2 anchor_{ 0.0f, 0.0f };     // 左上(0,0) / 中央(0.5,0.5) / 右下(1,1)

    // フリップ状態
    bool isFlipX_ = false;
    bool isFlipY_ = false;

    // 現在のテクスチャのピクセルサイズ（取得できない場合は 0）
    Vector2 textureSize_{ 0.0f, 0.0f };

    // 切り出し矩形（ピクセル指定）
    bool  useTexRect_ = false;
    Vector2 texRectLeftTop_{ 0.0f, 0.0f }; // px
    Vector2 texRectSize_{ 0.0f, 0.0f };    // px

    // 現在のテクスチャ解像度にスプライトサイズを合わせる
    void AdjustTextureSize(); 

    // アンカー反映で頂点ローカル座標を更新
    void ApplyAnchorToVertices();

public: //メンバ関数
    //デストラクタ
    ~Sprite() = default;

    //初期化
    void Initialize(Camera* camera, const std::string& textureName = "resources/uvChecker.png");
    //更新
    void Update(const bool& debug = true, const char* spriteName = "");
    // 描画
    void Draw();

    // ゲッター
    D3D12ResourceUtil* GetD3D12Resource() { return this->resource_.get(); }

    // サイズとアンカーの設定
    void SetSize(const float& width, const float& height);
    void SetAnchor(const float& ax, const float& ay);
    const Vector2& GetSize() const { return size_; }
    const Vector2& GetAnchor() const { return anchor_; }

    // 位置API（アンカー基準の座標を設定/取得）
    void SetPosition(const float& x, const float& y, const float& z = 0.0f);
    const Vector2 GetPosition2D() const;

    // 便利エイリアス
    void SetPositionTopLeft(const float& x, const float& y) { SetAnchor(0.0f, 0.0f); SetPosition(x, y); }
    void SetPositionCenter(const float& x, const float& y) { SetAnchor(0.5f, 0.5f); SetPosition(x, y); }

    // 回転
    const Vector3& GetRotation()const { return resource_->transform_.rotate; }
    void SetRotation(const float& rotate) { resource_->transform_.rotate = Vector3{ 0.0f,0.0f,rotate }; }

    // 色
    const Vector4& GetColor()const { return resource_->materialData_->color; }
    void SetColor(const Vector4& color) { resource_->materialData_->color = color; }

    // フリップAPI
    void SetFlip(bool flipX, bool flipY) { isFlipX_ = flipX; isFlipY_ = flipY; }
    void SetFlipX(bool flip) { isFlipX_ = flip; }
    void SetFlipY(bool flip) { isFlipY_ = flip; }
    bool IsFlipX() const { return isFlipX_; }
    bool IsFlipY() const { return isFlipY_; }

    //テクスチャ範囲指定（ピクセル）。成功時 true（テクスチャサイズ未取得時は false）
    bool SetTextureRectPixels(int x, int y, int w, int h, bool autoResize = false);
    // 範囲指定を解除（フルテクスチャに戻す）
    void ClearTextureRect();

    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
};