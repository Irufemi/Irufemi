#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <string>
#include "Renderer/VertexData.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/Vector4.h"
#include <vector>
#include <array>
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "../../Engine/Graphics/DirectX/ConstantBuffer.h"

// 蜑肴婿螳｣險
class Camera;
class IrufemiEngine;

/**
 * @class Skybox
 * @brief 繧ｹ繧ｫ繧､繝懊ャ繧ｯ繧ｹ縺ｮ謠冗判繧堤ｮ｡逅・☆繧九け繝ｩ繧ｹ
 */
class Skybox
{
public:
    // 繝・ヵ繧ｩ繝ｫ繝医・繝・け繧ｹ繝√Ε繝代せ
    static inline const std::string kDefaultTexturePath = "resources/rostock_laage_airport_4k.dds";

public: // 繝｡繝ｳ繝宣未謨ｰ
    // 繧ｳ繝ｳ繧ｹ繝医Λ繧ｯ繧ｿ
    Skybox();
    // 繝・せ繝医Λ繧ｯ繧ｿ
    ~Skybox();
    // 蛻晄悄蛹・
    void Initialize(Camera* camera, const std::string& textureName = kDefaultTexturePath);
    // 譖ｴ譁ｰ
    void Update();
    // 謠冗判
    void Draw();
    // 繝・ヰ繝・げ
    void Debug();
public: // 繝｡繝ｳ繝宣未謨ｰ(繧ｻ繝・ち繝ｼ/繧ｲ繝・ち繝ｼ)
    // engine繧ｻ繝・ち繝ｼ
    static void SetEngine(IrufemiEngine* engine) { engine_ = engine; }
    // ID3D12Resource髢｢騾｣繧ｲ繝・ち繝ｼ
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBufferView_; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBufferView_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle() const { return textureHandle_; }
    // index縺ｮ繧ｵ繧､繧ｺ蜿門ｾ・
    UINT GetIndexSize() const { return static_cast<UINT>(indexDataList_.size()); }
private: // 繝｡繝ｳ繝宣未謨ｰ(蜀・Κ繝倥Ν繝・
    // ID3D12Resource縺ｮ逕滓・
    void CreateResource();
    // ID3D12Resource縺ｮMap
    void MapResource();
    // Id3D12Resource縺ｮUnMap
    void UnMapResource();


private: // 繝｡繝ｳ繝仙､画焚(resource)
    /// vertex
    std::vector<VertexData> vertexDataList_{};
    VertexData* vertexData_ = nullptr;
    //鬆らせ繝・・繧ｿ繝舌ャ繝輔ぃ
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;

    /// index
    std::vector<uint32_t> indexDataList_{};
    uint32_t* indexData_ = nullptr;
    //鬆らせ繧､繝ｳ繝・ャ繧ｯ繧ｹ繝舌ャ繝輔ぃ
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
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
    };
    SkyboxTransformationMatrix transformationMatrix_{};
    ConstantBuffer<SkyboxTransformationMatrix> transformationBuffer_;

    // Material
    struct SkyboxMaterial {
        Vector4 color;
        float intensity;
        float padding[3];
    };
    ConstantBuffer<SkyboxMaterial> materialBuffer_;

    // texture
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_ = {};
    int selectedTextureIndex_ = 0;

    // 繧ｫ繝｡繝ｩ(繝昴う繝ｳ繧ｿ蜿ら・)
    Camera* camera_ = nullptr;
    // engine(繝昴う繝ｳ繧ｿ蜿ら・)
    static IrufemiEngine* engine_;

    // 陦悟・譖ｴ譁ｰ縺ｮ譛驕ｩ蛹也畑
    bool isDirty_ = true;
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};
    
    void MarkAsDirty() {
        for(int i=0; i<kMaxFramesInFlight; ++i) isDirtyBuffer_[i] = true;
    }

private:
    bool isDirtyBuffer_[kMaxFramesInFlight] = {true, true, true};
    
public:
    void SyncBeforeDraw();
};

