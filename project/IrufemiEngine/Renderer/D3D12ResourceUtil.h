#pragma once

#include <Windows.h>

#include <vector>
#include <d3d12.h>
#include <cstdint>
#include <wrl.h>

#include "math/Matrix4x4.h"
#include "math/Transform.h"
#include "math/VertexData.h"
#include "math/Material.h"
#include "math/ParticleMaterial.h"
#include "math/TransformationMatrix.h"
#include "math/DirectionalLight.h"
#include "math/CameraForGPU.h"
#include "math/LineVertexData.h"
#include "math/LineMaterial.h"
#include "manager/TextureManager.h"

// 前方宣言
class DirectXCommon;
class Camera;

class D3D12ResourceUtil {


public: //メンバ関数

    //デストラクタ
    ~D3D12ResourceUtil();

    //ID3D12Resourceを生成する
    void CreateResource();

    //バッファへの書き込みを開放
    void Map();

    //バッファへの書き込みを閉鎖
    void UnMap();

    void UpdateTransform3D(const Camera& camera);

    static void SetDirectXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
    DirectXCommon* GetDirectXCommon() { return dxCommon_; }

public: //メンバ変数

#pragma region Vertex

    // 頂点データリスト(position,texcoord,normal)
    std::vector<VertexData> vertexDataList_{};

    //頂点データ(position,texcoord,normal)
    VertexData* vertexData_ = nullptr;

#pragma endregion

#pragma region Index

    //頂点インデックスリスト
    std::vector<uint32_t> indexDataList_{};

    //頂点インデックス
    uint32_t* indexData_ = nullptr;

#pragma endregion

#pragma region Transform

    // transform(scale,rotate,translate)
    Transform transform_ = {
        {1.0f,1.0f,1.0f},   //scale
        {0.0f,0.0f,0.0f},   //rotate
        {0.0f,0.0f,0.0f}    //translate
    };

    // TransformationMatrix(WVP,world)
    TransformationMatrix transformationMatrix_{};

    // TransformationMatrixData(WVP,world)
    TransformationMatrix* transformationData_ = nullptr;

#pragma endregion

#pragma region Material

    //uvTransform(scale,rotate,translate)
    Transform uvTransform_{
        {1.0f,1.0f,1.0f},
        {0.0f,0.0f,0.0f},
        {0.0f,0.0f,0.0f}
    };

    //マテリアルデータ(color,enableLighting,uvTransform)
    Material* materialData_ = nullptr;

#pragma endregion

#pragma region Texture

    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_ = {};

#pragma endregion

#pragma region Buffer

    //頂点データバッファ
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    //頂点インデックスバッファ
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

#pragma endregion

#pragma region ID3D12Resource

    // 頂点データ用定数バッファ
    Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource_ = nullptr;
    // 頂点インデックス用定数バッファ
    Microsoft::WRL::ComPtr <ID3D12Resource> indexResource_ = nullptr;
    // 色用定数バッファ
    Microsoft::WRL::ComPtr <ID3D12Resource> materialResource_ = nullptr;
    // 拡縮回転移動行列用定数バッファ
    Microsoft::WRL::ComPtr <ID3D12Resource> transformationResource_ = nullptr;
#pragma endregion

#pragma region 外部参照

    static DirectXCommon* dxCommon_;

#pragma endregion
};

class D3D12ResourceUtilParticle {
public: //メンバ変数

#pragma region Vertex

    // 頂点データリスト(position,texcoord,normal)
    std::vector<VertexData> vertexDataList_{};

    //頂点データ(position,texcoord,normal)
    VertexData* vertexData_ = nullptr;

#pragma endregion

#pragma region Index

    //頂点インデックスリスト
    std::vector<uint32_t> indexDataList_{};

    //頂点インデックス
    uint32_t* indexData_ = nullptr;

#pragma endregion

#pragma region Material

    //uvTransform(scale,rotate,translate)
    Transform uvTransform_{
        {1.0f,1.0f,1.0f},
        {0.0f,0.0f,0.0f},
        {0.0f,0.0f,0.0f}
    };

    //マテリアルデータ(color,enableLighting,uvTransform)
    ParticleMaterial* materialData_ = nullptr;

#pragma endregion

#pragma region Texture

    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_ = {};

#pragma endregion

#pragma region Buffer

    //頂点データバッファ
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    //頂点インデックスバッファ
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

#pragma endregion

#pragma region ID3D12Resource

    // 頂点データ用定数バッファ
    Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource_ = nullptr;
    // 頂点インデックス用定数バッファ
    Microsoft::WRL::ComPtr <ID3D12Resource> indexResource_ = nullptr;
    // 色用定数バッファ
    Microsoft::WRL::ComPtr <ID3D12Resource> materialResource_ = nullptr;

#pragma endregion

#pragma region 外部参照

    static DirectXCommon* dxCommon_;

#pragma endregion

public: //メンバ関数

    static void SetDirectXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
    DirectXCommon* GetDirectXCommon() { return dxCommon_; }

    //デストラクタ
    ~D3D12ResourceUtilParticle();

    //ID3D12Resourceを生成する
    void CreateResource();

    //バッファへの書き込みを開放
    void Map();

    //バッファへの書き込みを閉鎖
    void UnMap();
};

class D3D12ResourceUtilLine {
public: // メンバ関数(リソース関連内部ヘルパ)
    // コンストラクタ
    D3D12ResourceUtilLine() = default;
    // デストラクタ
    ~D3D12ResourceUtilLine();

    // 一括Map
    void Map();

    // 一括UnMap
    void UnMap();

    // 一括CreateResource
    void CreateResource();

    static void SetDirectXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
    DirectXCommon* GetDirectXCommon() { return dxCommon_; }

public: // メンバ変数(D3D12リソース関連)

#pragma region Vertex

    //頂点データ(position)
    LineVertexData* vertexData_ = nullptr;

#pragma endregion

#pragma region Index

    //頂点インデックス
    uint32_t* indexData_ = nullptr;

#pragma endregion

#pragma region Transform

    // transform(scale,rotate,translate)
    Transform transform_ = {
        {1.0f,1.0f,1.0f},   //scale
        {0.0f,0.0f,0.0f},   //rotate
        {0.0f,0.0f,0.0f}    //translate
    };

    // TransformationMatrix(WVP,world)
    TransformationMatrix transformationMatrix_{};

    // TransformationMatrixData(WVP,world)
    TransformationMatrix* transformationData_ = nullptr;

#pragma endregion

#pragma region Material

    LineMaterial* materialData_ = nullptr;

#pragma endregion


#pragma region Buffer

    //頂点データバッファ
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    //頂点インデックスバッファ
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

#pragma endregion

#pragma region ID3D12Resource

    // 頂点データ用定数バッファ
    Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource_ = nullptr;
    // 頂点インデックス用定数バッファ
    Microsoft::WRL::ComPtr <ID3D12Resource> indexResource_ = nullptr;
    // 拡縮回転移動行列用定数バッファ
    Microsoft::WRL::ComPtr <ID3D12Resource> transformationResource_ = nullptr;
    // 色用定数バッファ
    Microsoft::WRL::ComPtr <ID3D12Resource> materialResource_ = nullptr;
#pragma endregion

#pragma region 外部参照

    static DirectXCommon* dxCommon_;

#pragma endregion
};