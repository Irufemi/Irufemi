#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <string>
#include "math/VertexData.h"
#include "math/Transform.h"
#include "math/Matrix4x4.h"
#include "math/Vector4.h"
#include <vector>

// 前方宣言
class Camera;
class IrufemiEngine;

class Skybox
{
public: // メンバ関数
    // コンストラクタ
    Skybox();
    // デストラクタ
    ~Skybox();
    // 初期化
    void Initialize(Camera* camera, const std::string& textureName = "resources/rostock_laage_airport_4k.dds");
    // 更新
    void Update();
    // 描画
    void Draw();
    // デバッグ
    void Debug();
public: // メンバ関数(セッター/ゲッター)
    // engineセッター
    static void SetEngine(IrufemiEngine* engine) { engine_ = engine; }
    // ID3D12Resource関連ゲッター
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBufferView_; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBufferView_; }
    ID3D12Resource* GetMaterialResource() const { return materialResource_.Get(); }
    ID3D12Resource* GetTransformationResource() const { return transformationResource_.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle() const { return textureHandle_; }
    // indexのサイズ取得
    UINT GetIndexSize() const { return static_cast<UINT>(indexDataList_.size()); }
private: // メンバ関数(内部ヘルパ)
    // ID3D12Resourceの生成
    void CreateResource();
    // ID3D12ResourceのMap
    void MapResource();
    // Id3D12ResourceのUnMap
    void UnMapResource();


private: // メンバ変数(resource)
    /// vertex
    std::vector<VertexData> vertexDataList_{};
    VertexData* vertexData_ = nullptr;
    //頂点データバッファ
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;

    /// index
    std::vector<uint32_t> indexDataList_{};
    uint32_t* indexData_ = nullptr;
    //頂点インデックスバッファ
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;

    /// Transform
    // transform(scale,rotate,translate)
    Transform transform_ = {
        {500.0f,500.0f,500.0f},   //scale
        {0.0f,0.0f,0.0f},   //rotate
        {0.0f,0.0f,0.0f}    //translate
    };
    struct SkyboxTransformationMatrix {
        Matrix4x4 WVP;
    };
    SkyboxTransformationMatrix transformationMatrix_{};
    SkyboxTransformationMatrix* transformationData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_ = nullptr;

    // Material
    struct SkyboxMaterial {
        Vector4 color;
    };
    SkyboxMaterial* materialData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;

    // texture
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_ = {};
    int selectedTextureIndex_ = 0;

    // カメラ(ポインタ参照)
    Camera* camera_ = nullptr;
    // engine(ポインタ参照)
    static IrufemiEngine* engine_;

};

