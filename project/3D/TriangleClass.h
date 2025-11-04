#pragma once

#include <vector>
#include <array>
#include <d3d12.h>

#include "../math/shape/Triangle.h"
#include "../source/D3D12ResourceUtil.h"
#include "../camera/Camera.h"
#include <wrl.h>
#include <memory>

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

class TriangleClass {
protected: //メンバ変数

    Triangle info_{};
    
    std::unique_ptr<D3D12ResourceUtil> resource_ = nullptr;

    int selectedTextureIndex_ = 0;

    // ポインタ参照

    Camera* camera_ = nullptr;

    static TextureManager* textureManager_;

    static DrawManager* drawManager_;

    static DebugUI* ui_;

public: //メンバ関数
    //デストラクタ
    ~TriangleClass() = default;

    //初期化
    void Initialize(Camera* camera,const std::string& textureName = "resources/uvChecker.png");
    //更新
    void Update(const char* triangleName = "");

    // 描画
    void Draw();

    //ゲッター
    D3D12ResourceUtil* GetD3D12Resource() { return this->resource_.get(); }

    // Triangleの情報を取得
    Triangle GetInfo() const { return info_; }

    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    
};

