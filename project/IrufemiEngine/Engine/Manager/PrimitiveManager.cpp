#include "PrimitiveManager.h"
#include "Renderer/Core/BaseResource.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector2.h"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <utility>

PrimitiveManager* PrimitiveManager::instance = nullptr;

PrimitiveManager* PrimitiveManager::GetInstance() {
    if (!instance) {
        instance = new PrimitiveManager();
    }
    return instance;
}

void PrimitiveManager::Finalize() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

const PrimitiveData& PrimitiveManager::GetPrimitiveData(PrimitiveType type) {
    auto it = cpuCache_.find(type);
    if (it != cpuCache_.end()) {
        return it->second;
    }

    PrimitiveData data;
    switch (type) {
    case PrimitiveType::Triangle: data = CreateTriangle(); break;
    case PrimitiveType::Plane:    data = CreatePlane(); break;
    case PrimitiveType::Cube:     data = CreateCube(1.0f, 1.0f, 1.0f); break;
    case PrimitiveType::Sphere:   data = CreateSphere(0.5f, 32); break;
    case PrimitiveType::Cylinder: data = CreateCylinder(0.5f, 1.0f, 32); break;
    case PrimitiveType::Tetra:    data = CreateTetra(); break;
    case PrimitiveType::Circle:   data = CreateCircle(0.5f, 32); break;
    case PrimitiveType::Ring:     data = CreateRing(0.2f, 0.5f, 0.0f, 360.0f, 32, false); break;
    case PrimitiveType::Skybox:   data = CreateCube(1.0f, 1.0f, 1.0f); break;
    default: break;
    }

    cpuCache_[type] = std::move(data);
    return cpuCache_[type];
}

const std::vector<VertexData>& PrimitiveManager::GetVertices(PrimitiveType type) {
    return GetPrimitiveData(type).vertices;
}

const PrimitiveResource& PrimitiveManager::GetStandardResource(PrimitiveType type) {
    auto it = gpuCache_.find(type);
    if (it != gpuCache_.end()) {
        return it->second;
    }

    const auto& data = GetPrimitiveData(type);
    PrimitiveResource resource;
    CreateGPUResource(data, resource);
    gpuCache_[type] = std::move(resource);
    return gpuCache_[type];
}

void PrimitiveManager::CreateGPUResource(const PrimitiveData& data, PrimitiveResource& resource) {
    auto* dxCommon = BaseResource::GetDirectXCommon();
    if (!dxCommon) return;

    resource.indexCount = static_cast<uint32_t>(data.indices.size());
    
    resource.vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * data.vertices.size());
    resource.indexResource = dxCommon->CreateBufferResource(sizeof(uint32_t) * data.indices.size());

    // データの転送
    VertexData* vertexMapped = nullptr;
    resource.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexMapped));
    std::copy(data.vertices.begin(), data.vertices.end(), vertexMapped);
    resource.vertexResource->Unmap(0, nullptr);

    uint32_t* indexMapped = nullptr;
    resource.indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexMapped));
    std::copy(data.indices.begin(), data.indices.end(), indexMapped);
    resource.indexResource->Unmap(0, nullptr);

    // View の作成
    resource.vertexBufferView.BufferLocation = resource.vertexResource->GetGPUVirtualAddress();
    resource.vertexBufferView.StrideInBytes = sizeof(VertexData);
    resource.vertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * data.vertices.size());

    resource.indexBufferView.BufferLocation = resource.indexResource->GetGPUVirtualAddress();
    resource.indexBufferView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * data.indices.size());
    resource.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

PrimitiveData PrimitiveManager::CreateTriangle() {
    PrimitiveData data;
    data.vertices = {
        { {  0.0f,  0.5f, 0.0f, 1.0f }, { 0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
        { {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
        { { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }
    };
    data.indices = { 0, 1, 2 };
    return data;
}

PrimitiveData PrimitiveManager::CreatePlane(float width, float height) {
    PrimitiveData data;
    float hx = width * 0.5f;
    float hy = height * 0.5f;

    data.vertices = {
        { { -hx, -hy, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }, // v0
        { {  hx, -hy, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }, // v1
        { {  hx,  hy, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // v2
        { { -hx,  hy, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // v3
    };
    data.indices = { 0, 2, 1, 0, 3, 2 };
    return data;
}

PrimitiveData PrimitiveManager::CreateCube(float width, float height, float depth) {
    PrimitiveData data;
    const float hx = width * 0.5f;
    const float hy = height * 0.5f;
    const float hz = depth * 0.5f;

    data.vertices = {
        // 前面 (-Z)
        { { -hx, -hy, -hz, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }, // 0
        { { -hx,  hy, -hz, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // 1
        { {  hx, -hy, -hz, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }, // 2
        { {  hx,  hy, -hz, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // 3
        // 背面 (+Z)
        { {  hx, -hy,  hz, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }, // 4
        { {  hx,  hy,  hz, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }, // 5
        { { -hx, -hy,  hz, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }, // 6
        { { -hx,  hy,  hz, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }, // 7
        // 左面 (-X)
        { { -hx, -hy,  hz, 1.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f } }, // 8
        { { -hx,  hy,  hz, 1.0f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // 9
        { { -hx, -hy, -hz, 1.0f }, { 1.0f, 1.0f }, { -1.0f, 0.0f, 0.0f } }, // 10
        { { -hx,  hy, -hz, 1.0f }, { 1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // 11
        // 右面 (+X)
        { {  hx, -hy, -hz, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }, // 12
        { {  hx,  hy, -hz, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } }, // 13
        { {  hx, -hy,  hz, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }, // 14
        { {  hx,  hy,  hz, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } }, // 15
        // 下面 (-Y)
        { { -hx, -hy, -hz, 1.0f }, { 0.0f, 1.0f }, { 0.0f, -1.0f, 0.0f } }, // 16
        { {  hx, -hy, -hz, 1.0f }, { 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f } }, // 17
        { { -hx, -hy,  hz, 1.0f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } }, // 18
        { {  hx, -hy,  hz, 1.0f }, { 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } }, // 19
        // 上面 (+Y)
        { { -hx,  hy,  hz, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } }, // 20
        { {  hx,  hy,  hz, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } }, // 21
        { { -hx,  hy, -hz, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }, // 22
        { {  hx,  hy, -hz, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }  // 23
    };
    data.indices = {
        0, 1, 2, 2, 1, 3, // 前面
        4, 5, 6, 6, 5, 7, // 背面
        8, 9, 10, 10, 9, 11, // 左面
        12, 13, 14, 14, 13, 15, // 右面
        16, 17, 18, 18, 17, 19, // 下面
        20, 21, 22, 22, 21, 23  // 上面
    };
    return data;
}

PrimitiveData PrimitiveManager::CreateSphere(float radius, uint32_t subdivision) {
    PrimitiveData data;
    const float pi = std::numbers::pi_v<float>;
    const float latEvery = pi / static_cast<float>(subdivision);
    const float lonEvery = 2.0f * pi / static_cast<float>(subdivision);

    for (uint32_t latIndex = 0; latIndex <= subdivision; ++latIndex) {
        float lat = -pi / 2.0f + latEvery * latIndex;
        for (uint32_t lonIndex = 0; lonIndex <= subdivision; ++lonIndex) {
            float lon = lonIndex * lonEvery;
            VertexData v;
            v.position = {
                radius * std::cos(lat) * std::cos(lon),
                radius * std::sin(lat),
                radius * std::cos(lat) * std::sin(lon),
                1.0f
            };
            v.texcoord = {
                static_cast<float>(lonIndex) / subdivision,
                1.0f - static_cast<float>(latIndex) / subdivision
            };
            v.normal = { v.position.x / radius, v.position.y / radius, v.position.z / radius };
            data.vertices.push_back(v);
        }
    }

    for (uint32_t latIndex = 0; latIndex < subdivision; ++latIndex) {
        for (uint32_t lonIndex = 0; lonIndex < subdivision; ++lonIndex) {
            uint32_t base = (subdivision + 1) * latIndex + lonIndex;
            data.indices.push_back(base);
            data.indices.push_back(base + subdivision + 1);
            data.indices.push_back(base + 1);
            data.indices.push_back(base + subdivision + 1);
            data.indices.push_back(base + subdivision + 2);
            data.indices.push_back(base + 1);
        }
    }
    return data;
}

PrimitiveData PrimitiveManager::CreateCylinder(float radius, float height, uint32_t segments) {
    PrimitiveData data;
    const float pi = std::numbers::pi_v<float>;
    const float radianPerDivide = 2.0f * pi / static_cast<float>(segments);

    for (uint32_t i = 0; i < segments; ++i) {
        float rad = static_cast<float>(i) * radianPerDivide;
        float radNext = static_cast<float>(i + 1) * radianPerDivide;

        float s = std::sin(rad);
        float c = std::cos(rad);
        float sNext = std::sin(radNext);
        float cNext = std::cos(radNext);

        float u = static_cast<float>(i) / segments;
        float uNext = static_cast<float>(i + 1) / segments;

        uint32_t base = static_cast<uint32_t>(data.vertices.size());
        
        data.vertices.push_back({ { c * radius, -height * 0.5f, s * radius, 1.0f }, { u, 1.0f }, { c, 0.0f, s } });
        data.vertices.push_back({ { c * radius,  height * 0.5f, s * radius, 1.0f }, { u, 0.0f }, { c, 0.0f, s } });
        data.vertices.push_back({ { cNext * radius, -height * 0.5f, sNext * radius, 1.0f }, { uNext, 1.0f }, { cNext, 0.0f, sNext } });
        data.vertices.push_back({ { cNext * radius,  height * 0.5f, sNext * radius, 1.0f }, { uNext, 0.0f }, { cNext, 0.0f, sNext } });

        data.indices.push_back(base);
        data.indices.push_back(base + 1);
        data.indices.push_back(base + 2);
        data.indices.push_back(base + 1);
        data.indices.push_back(base + 3);
        data.indices.push_back(base + 2);
    }
    return data;
}

PrimitiveData PrimitiveManager::CreateRing(float innerRadius, float outerRadius, float startAngle, float endAngle, uint32_t segments, bool verticalUV) {
    PrimitiveData data;
    const float pi = std::numbers::pi_v<float>;
    float startRad = startAngle * (pi / 180.0f);
    float endRad = endAngle * (pi / 180.0f);
    if (endRad <= startRad) endRad += 2.0f * pi;
    float arc = endRad - startRad;
    float radianPerDivide = arc / static_cast<float>(segments);

    for (uint32_t i = 0; i < segments; ++i) {
        float a0 = startRad + i * radianPerDivide;
        float a1 = startRad + (i + 1) * radianPerDivide;

        float s0 = std::sin(a0), c0 = std::cos(a0);
        float s1 = std::sin(a1), c1 = std::cos(a1);

        float u = static_cast<float>(i) / segments;
        float uNext = static_cast<float>(i + 1) / segments;

        uint32_t base = static_cast<uint32_t>(data.vertices.size());
        VertexData v0, v1, v2, v3;
        v0.position = { c0 * outerRadius, s0 * outerRadius, 0.0f, 1.0f };
        v1.position = { c1 * outerRadius, s1 * outerRadius, 0.0f, 1.0f };
        v2.position = { c0 * innerRadius, s0 * innerRadius, 0.0f, 1.0f };
        v3.position = { c1 * innerRadius, s1 * innerRadius, 0.0f, 1.0f };

        if (verticalUV) {
            v0.texcoord = { 0.0f, u }; v1.texcoord = { 0.0f, uNext };
            v2.texcoord = { 1.0f, u }; v3.texcoord = { 1.0f, uNext };
        } else {
            v0.texcoord = { u, 0.0f }; v1.texcoord = { uNext, 0.0f };
            v2.texcoord = { u, 1.0f }; v3.texcoord = { uNext, 1.0f };
        }
        v0.normal = v1.normal = v2.normal = v3.normal = { 0.0f, 0.0f, -1.0f };

        data.vertices.push_back(v0); data.vertices.push_back(v1);
        data.vertices.push_back(v2); data.vertices.push_back(v3);

        data.indices.push_back(base); data.indices.push_back(base + 2); data.indices.push_back(base + 1);
        data.indices.push_back(base + 1); data.indices.push_back(base + 2); data.indices.push_back(base + 3);
    }
    return data;
}

PrimitiveData PrimitiveManager::CreateTetra() {
    PrimitiveData data;
    const float s = 0.5f;
    const float R = 1.0f / std::sqrt(3.0f);
    const float baseToApex = std::sqrt(2.0f / 3.0f);
    const float b = baseToApex / 4.0f;
    const float a = 3.0f * b;

    Vector3 apex = { 0.0f, a * s, 0.0f };
    Vector3 v0 = { 0.0f, -b * s, R * s };
    Vector3 v1 = { -0.5f * s, -b * s, -R * 0.5f * s };
    Vector3 v2 = { 0.5f * s, -b * s, -R * 0.5f * s };

    std::vector<std::vector<Vector3>> faces = { { v0, v1, v2 }, { apex, v0, v1 }, { apex, v1, v2 }, { apex, v2, v0 } };

    for (const auto& face : faces) {
        uint32_t base = static_cast<uint32_t>(data.vertices.size());
        Vector3 p0 = face[0], p1 = face[1], p2 = face[2];
        Vector3 e0 = { p1.x - p0.x, p1.y - p0.y, p1.z - p0.z };
        Vector3 e1 = { p2.x - p0.x, p2.y - p0.y, p2.z - p0.z };
        // Simplified Cross and Normalize
        Vector3 n = { e0.y * e1.z - e0.z * e1.y, e0.z * e1.x - e0.x * e1.z, e0.x * e1.y - e0.y * e1.x };
        float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        n = { n.x / len, n.y / len, n.z / len };

        data.vertices.push_back({ { p0.x, p0.y, p0.z, 1.0f }, { 0.5f, 0.0f }, n });
        data.vertices.push_back({ { p1.x, p1.y, p1.z, 1.0f }, { 0.0f, 1.0f }, n });
        data.vertices.push_back({ { p2.x, p2.y, p2.z, 1.0f }, { 1.0f, 1.0f }, n });

        Vector3 centroid = { (p0.x + p1.x + p2.x) / 3.0f, (p0.y + p1.y + p2.y) / 3.0f, (p0.z + p1.z + p2.z) / 3.0f };
        if (n.x * centroid.x + n.y * centroid.y + n.z * centroid.z >= 0.0f) {
            data.indices.push_back(base); data.indices.push_back(base + 1); data.indices.push_back(base + 2);
        } else {
            data.indices.push_back(base + 2); data.indices.push_back(base + 1); data.indices.push_back(base );
        }
    }
    return data;
}

PrimitiveData PrimitiveManager::CreateCircle(float radius, uint32_t segments) {
    PrimitiveData data;
    const float pi = std::numbers::pi_v<float>;
    data.vertices.push_back({ { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.5f, 0.5f }, { 0.0f, 0.0f, -1.0f } });
    for (uint32_t i = 0; i <= segments; ++i) {
        float rad = 2.0f * pi * i / segments;
        float c = std::cos(rad), s = std::sin(rad);
        data.vertices.push_back({ { c * radius, s * radius, 0.0f, 1.0f }, { c * 0.5f + 0.5f, s * 0.5f + 0.5f }, { 0.0f, 0.0f, -1.0f } });
    }
    for (uint32_t i = 0; i < segments; ++i) {
        data.indices.push_back(0); data.indices.push_back(i + 1); data.indices.push_back(i + 2);
    }
    return data;
}