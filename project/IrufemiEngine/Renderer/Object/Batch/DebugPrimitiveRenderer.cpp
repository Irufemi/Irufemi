#include "DebugPrimitiveRenderer.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/DirectX/DescriptorPool.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/IrufemiEngine.h"

DebugPrimitiveRenderer::~DebugPrimitiveRenderer() {
    if (srvAllocator_ && dx_) {
        for (uint32_t& idx : sphereSrvIndex_) {
            if (idx != UINT32_MAX) {
                srvAllocator_->FreeAfterFence(idx, dx_->GetFenceValue());
                idx = UINT32_MAX;
            }
        }
        for (uint32_t& idx : cubeSrvIndex_) {
            if (idx != UINT32_MAX) {
                srvAllocator_->FreeAfterFence(idx, dx_->GetFenceValue());
                idx = UINT32_MAX;
            }
        }
    }
}

void DebugPrimitiveRenderer::Initialize(DirectXCommon* dx, DrawManager* drawM, DescriptorPool* srvAlloc) {
    dx_ = dx;
    drawManager_ = drawM;
    srvAllocator_ = srvAlloc;

    sphereSrvIndex_.fill(UINT32_MAX);
    cubeSrvIndex_.fill(UINT32_MAX);

    sphereInstances_.resize(maxSphereInstances_);
    cubeInstances_.resize(maxCubeInstances_);

    CreateSphereResource();
    CreateCubeResource();
}

void DebugPrimitiveRenderer::CreateSphereResource() {
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;

    const int segments = 32;
    for (int ring = 0; ring < 3; ++ring) {
        for (int i = 0; i < segments; ++i) {
            float angle = static_cast<float>(i) / segments * 2.0f * 3.14159265359f;
            float c = std::cos(angle);
            float s = std::sin(angle);

            Vector3 pos;
            if (ring == 0) pos = { c, s, 0.0f };
            else if (ring == 1) pos = { 0.0f, c, s };
            else pos = { s, 0.0f, c };
            
            VertexData vd{};
            vd.position = { pos.x, pos.y, pos.z, 1.0f };
            vd.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            vertices.push_back(vd);

            uint32_t baseIdx = ring * segments;
            indices.push_back(baseIdx + i);
            indices.push_back(baseIdx + (i + 1) % segments);
        }
    }

    sphereIndexCount_ = static_cast<uint32_t>(indices.size());

    // Create Vertex Buffer
    size_t vbSize = vertices.size() * sizeof(VertexData);
    sphereVertexResource_ = dx_->CreateBufferResource(vbSize);
    VertexData* mappedVB = nullptr;
    sphereVertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVB));
    std::memcpy(mappedVB, vertices.data(), vbSize);
    sphereVertexResource_->Unmap(0, nullptr);

    sphereVBV_.BufferLocation = sphereVertexResource_->GetGPUVirtualAddress();
    sphereVBV_.SizeInBytes = static_cast<UINT>(vbSize);
    sphereVBV_.StrideInBytes = sizeof(VertexData);

    // Create Index Buffer
    size_t ibSize = indices.size() * sizeof(uint32_t);
    sphereIndexResource_ = dx_->CreateBufferResource(ibSize);
    uint32_t* mappedIB = nullptr;
    sphereIndexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedIB));
    std::memcpy(mappedIB, indices.data(), ibSize);
    sphereIndexResource_->Unmap(0, nullptr);

    sphereIBV_.BufferLocation = sphereIndexResource_->GetGPUVirtualAddress();
    sphereIBV_.SizeInBytes = static_cast<UINT>(ibSize);
    sphereIBV_.Format = DXGI_FORMAT_R32_UINT;
}

void DebugPrimitiveRenderer::CreateCubeResource() {
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices = {
        0,1, 1,2, 2,3, 3,0, // Bottom
        4,5, 5,6, 6,7, 7,4, // Top
        0,4, 1,5, 2,6, 3,7  // Pillars
    };

    Vector3 positions[8] = {
        { -0.5f, -0.5f, -0.5f },
        {  0.5f, -0.5f, -0.5f },
        {  0.5f, -0.5f,  0.5f },
        { -0.5f, -0.5f,  0.5f },
        { -0.5f,  0.5f, -0.5f },
        {  0.5f,  0.5f, -0.5f },
        {  0.5f,  0.5f,  0.5f },
        { -0.5f,  0.5f,  0.5f },
    };

    for (int i = 0; i < 8; ++i) {
        VertexData vd{};
        vd.position = { positions[i].x, positions[i].y, positions[i].z, 1.0f };
        vd.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        vertices.push_back(vd);
    }

    cubeIndexCount_ = static_cast<uint32_t>(indices.size());

    // Create Vertex Buffer
    size_t vbSize = vertices.size() * sizeof(VertexData);
    cubeVertexResource_ = dx_->CreateBufferResource(vbSize);
    VertexData* mappedVB = nullptr;
    cubeVertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVB));
    std::memcpy(mappedVB, vertices.data(), vbSize);
    cubeVertexResource_->Unmap(0, nullptr);

    cubeVBV_.BufferLocation = cubeVertexResource_->GetGPUVirtualAddress();
    cubeVBV_.SizeInBytes = static_cast<UINT>(vbSize);
    cubeVBV_.StrideInBytes = sizeof(VertexData);

    // Create Index Buffer
    size_t ibSize = indices.size() * sizeof(uint32_t);
    cubeIndexResource_ = dx_->CreateBufferResource(ibSize);
    uint32_t* mappedIB = nullptr;
    cubeIndexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedIB));
    std::memcpy(mappedIB, indices.data(), ibSize);
    cubeIndexResource_->Unmap(0, nullptr);

    cubeIBV_.BufferLocation = cubeIndexResource_->GetGPUVirtualAddress();
    cubeIBV_.SizeInBytes = static_cast<UINT>(ibSize);
    cubeIBV_.Format = DXGI_FORMAT_R32_UINT;
}

void DebugPrimitiveRenderer::ClearInstances() {
    activeSphereCount_ = 0;
    activeCubeCount_ = 0;
}

void DebugPrimitiveRenderer::AddSphere(const Vector3& center, float radius, const Vector4& color) {
    if (activeSphereCount_ < maxSphereInstances_) {
        auto& instance = sphereInstances_[activeSphereCount_];
        instance.world = Math::MakeScaleMatrix({radius, radius, radius}) * Math::MakeTranslateMatrix(center);
        instance.color = color;
        activeSphereCount_++;
    }
}

void DebugPrimitiveRenderer::AddCube(const Matrix4x4& transform, const Vector4& color) {
    if (activeCubeCount_ < maxCubeInstances_) {
        auto& instance = cubeInstances_[activeCubeCount_];
        instance.world = transform;
        instance.color = color;
        activeCubeCount_++;
    }
}

void DebugPrimitiveRenderer::Update() {
    BuildInstanceBuffer();
}

void DebugPrimitiveRenderer::EnsureInstancingSRVs() {
    uint32_t frameIndex = dx_->GetFrameIndex();
    
    // Sphere
    if (sphereSrvIndex_[frameIndex] == UINT32_MAX) {
        sphereSrvIndex_[frameIndex] = srvAllocator_->Allocate();
        sphereSrvGPU_[frameIndex] = srvAllocator_->GetGPUHandle(sphereSrvIndex_[frameIndex]);
    }
    
    // Cube
    if (cubeSrvIndex_[frameIndex] == UINT32_MAX) {
        cubeSrvIndex_[frameIndex] = srvAllocator_->Allocate();
        cubeSrvGPU_[frameIndex] = srvAllocator_->GetGPUHandle(cubeSrvIndex_[frameIndex]);
    }
}

void DebugPrimitiveRenderer::BuildInstanceBuffer() {
    if (activeSphereCount_ == 0 && activeCubeCount_ == 0) return;
    
    uint32_t frameIndex = dx_->GetFrameIndex();
    lastUpdateFrameIndex_ = frameIndex;

    Camera* activeCam = dx_->GetEngine()->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    // Sphere Buffer
    if (activeSphereCount_ > 0) {
        if (activeSphereCount_ > sphereInstanceCapacity_[frameIndex]) {
            if (sphereInstanceBuffer_[frameIndex]) {
                sphereInstanceBuffer_[frameIndex]->Unmap(0, nullptr);
            }
            sphereInstanceCapacity_[frameIndex] = static_cast<uint32_t>(activeSphereCount_);
            size_t size = sizeof(InstanceData) * sphereInstanceCapacity_[frameIndex];
            sphereInstanceBuffer_[frameIndex] = dx_->CreateBufferResource(size);
            sphereInstanceBuffer_[frameIndex]->Map(0, nullptr, reinterpret_cast<void**>(&sphereInstanceDataMap_[frameIndex]));
            
            EnsureInstancingSRVs();
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Buffer.NumElements = sphereInstanceCapacity_[frameIndex];
            srvDesc.Buffer.StructureByteStride = sizeof(InstanceData);
            
            dx_->GetDevice()->CreateShaderResourceView(sphereInstanceBuffer_[frameIndex].Get(), &srvDesc, srvAllocator_->GetCPUHandle(sphereSrvIndex_[frameIndex]));
        }

        for (size_t i = 0; i < activeSphereCount_; ++i) {
            sphereInstanceDataMap_[frameIndex][i].world = sphereInstances_[i].world;
            sphereInstanceDataMap_[frameIndex][i].color = sphereInstances_[i].color;
        }
    }

    // Cube Buffer
    if (activeCubeCount_ > 0) {
        if (activeCubeCount_ > cubeInstanceCapacity_[frameIndex]) {
            if (cubeInstanceBuffer_[frameIndex]) {
                cubeInstanceBuffer_[frameIndex]->Unmap(0, nullptr);
            }
            cubeInstanceCapacity_[frameIndex] = static_cast<uint32_t>(activeCubeCount_);
            size_t size = sizeof(InstanceData) * cubeInstanceCapacity_[frameIndex];
            cubeInstanceBuffer_[frameIndex] = dx_->CreateBufferResource(size);
            cubeInstanceBuffer_[frameIndex]->Map(0, nullptr, reinterpret_cast<void**>(&cubeInstanceDataMap_[frameIndex]));
            
            EnsureInstancingSRVs();
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Buffer.NumElements = cubeInstanceCapacity_[frameIndex];
            srvDesc.Buffer.StructureByteStride = sizeof(InstanceData);
            
            dx_->GetDevice()->CreateShaderResourceView(cubeInstanceBuffer_[frameIndex].Get(), &srvDesc, srvAllocator_->GetCPUHandle(cubeSrvIndex_[frameIndex]));
        }

        for (size_t i = 0; i < activeCubeCount_; ++i) {
            cubeInstanceDataMap_[frameIndex][i].world = cubeInstances_[i].world;
            cubeInstanceDataMap_[frameIndex][i].color = cubeInstances_[i].color;
        }
    }
}

void DebugPrimitiveRenderer::Draw() {
    uint32_t frameIndex = dx_->GetFrameIndex();

    if (activeSphereCount_ > 0) {
        RenderPackets::DebugPrimitivePacket packet{};
        packet.vertexBufferView = sphereVBV_;
        packet.indexBufferView = sphereIBV_;
        packet.indexCount = sphereIndexCount_;
        packet.instanceCount = static_cast<UINT>(activeSphereCount_);
        packet.instancingSrvHandleGPU = sphereSrvGPU_[frameIndex];
        drawManager_->SubmitDebugPrimitive(packet);
    }

    if (activeCubeCount_ > 0) {
        RenderPackets::DebugPrimitivePacket packet{};
        packet.vertexBufferView = cubeVBV_;
        packet.indexBufferView = cubeIBV_;
        packet.indexCount = cubeIndexCount_;
        packet.instanceCount = static_cast<UINT>(activeCubeCount_);
        packet.instancingSrvHandleGPU = cubeSrvGPU_[frameIndex];
        drawManager_->SubmitDebugPrimitive(packet);
    }
}
