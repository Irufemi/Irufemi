#pragma once

#include "Renderer/VertexData.h"
#include "Engine/Core/Type/PrimitiveType.h"
#include "Renderer/Particle/Data/Particle.h"
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <d3d12.h>
#include <wrl.h>

struct PrimitiveData {
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
};

struct PrimitiveResource {
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    uint32_t indexCount;
};

class PrimitiveManager
{
public:
    /// <summary>
    /// インスタンスを取得する
    /// </summary>
    static PrimitiveManager* GetInstance();

    /// <summary>
    /// 全リソースを解放する
    /// </summary>
    static void Finalize();

    /// <summary>
    /// 指定した形状のプリミティブデータを取得する（CPUキャッシュ）
    /// </summary>
    const PrimitiveData& GetPrimitiveData(PrimitiveType type);

    /// <summary>
    /// 指定した形状の頂点データのみを取得する
    /// </summary>
    const std::vector<VertexData>& GetVertices(PrimitiveType type);

    /// <summary>
    /// 指定した形状の GPU リソース（BufferView）を取得する（GPUキャッシュ）
    /// 標準設定（サイズ1.0等）のバッファを共有します。
    /// </summary>
    const PrimitiveResource& GetStandardResource(PrimitiveType type);

    // 個別生成用（キャッシュしない。特殊なパラメータが必要な場合用）
    static PrimitiveData CreateSphere(float radius, uint32_t subdivision);
    static PrimitiveData CreateCube(float width, float height, float depth);
    static PrimitiveData CreateCylinder(float radius, float height, uint32_t segments);
    static PrimitiveData CreateCone(float radius, float height, uint32_t segments);
    static PrimitiveData CreateTorus(float majorRadius, float minorRadius, uint32_t majorSegments, uint32_t minorSegments);
    static PrimitiveData CreateIcoSphere(float radius, uint32_t subdivision);
    static PrimitiveData CreateGrid(float width, float height, uint32_t xSegments, uint32_t ySegments);
    static PrimitiveData CreateRing(float innerRadius, float outerRadius, float startAngle, float endAngle, uint32_t segments, bool verticalUV);
    static PrimitiveData CreatePlane(float width = 1.0f, float height = 1.0f);
    static PrimitiveData CreateTriangle();
    static PrimitiveData CreateTetra();
    static PrimitiveData CreateCircle(float radius, uint32_t segments);

private:
    PrimitiveManager() = default;
    ~PrimitiveManager() = default;
    PrimitiveManager(const PrimitiveManager&) = delete;
    PrimitiveManager& operator=(const PrimitiveManager&) = delete;

    // リソース生成補助
    void CreateGPUResource(const PrimitiveData& data, PrimitiveResource& resource);

private:
    static PrimitiveManager* instance;

    std::map<PrimitiveType, PrimitiveData> cpuCache_;
    std::map<PrimitiveType, PrimitiveResource> gpuCache_;
};

