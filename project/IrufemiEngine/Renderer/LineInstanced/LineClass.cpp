#include "LineClass.h"

#include "Application/camera/Camera.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/DirectX/DescriptorPool.h"
#include "Engine/Core/Math/Geometry/Math.h"

DirectXCommon* Line3DRegion::dx_ = nullptr;
DrawManager* Line3DRegion::drawManager_ = nullptr;
DescriptorPool* Line3DRegion::s_srvAllocator_ = nullptr;

void Line3DRegion::Initialize(Camera* camera) {
    camera_ = camera;
    instances_.resize(maxInstances_);

    baseLineResource_ = std::make_unique<D3D12ResourceUtilLine>();
    baseLineResource_->CreateResource();
    baseLineResource_->Map();

    // 基準となる線の頂点データ (0,0,0) -> (1,0,0)
    baseLineResource_->vertexData_[0] = { {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f} };
    baseLineResource_->vertexData_[1] = { {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f} };

    baseLineResource_->indexData_[0] = 0;
    baseLineResource_->indexData_[1] = 1;

    baseLineResource_->vertexBufferView_.BufferLocation = baseLineResource_->vertexResource_->GetGPUVirtualAddress();
    baseLineResource_->vertexBufferView_.StrideInBytes = sizeof(LineVertexData);
    baseLineResource_->vertexBufferView_.SizeInBytes = sizeof(LineVertexData) * 2;

    baseLineResource_->indexBufferView_.BufferLocation = baseLineResource_->indexResource_->GetGPUVirtualAddress();
    baseLineResource_->indexBufferView_.SizeInBytes = sizeof(uint32_t) * 2;
    baseLineResource_->indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}

void Line3DRegion::Update() {
    activeCount_ = 0;
    for (size_t i = 0; i < instances_.size(); ++i) {
        if (instances_[i].active) {
            if (activeCount_ < i) {
                instances_[activeCount_] = instances_[i];
            }
            activeCount_++;
        }
    }
}

void Line3DRegion::AddInstance(const Vector3& start, const Vector3& end, const Vector4& color, float life) {
    if (activeCount_ < maxInstances_) {
        auto& instance = instances_[activeCount_];
        instance.start = start;
        instance.end = end;
        instance.color = color;
        instance.life = life;
        instance.age = 0.0f;
        instance.active = true;
        activeCount_++;
    }
}

void Line3DRegion::ClearInstances() {
    for (size_t i = 0; i < activeCount_; ++i) {
        instances_[i].active = false;
    }
    activeCount_ = 0;
}

void Line3DRegion::BuildInstanceBuffer(bool force) {
    if (activeCount_ == 0 && !force) return;

    CreateOrResizeInstanceBuffer(static_cast<uint32_t>(activeCount_));
    if (!instanceBuffer_ || !instanceData_) return;

    const Matrix4x4& viewProjection = Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix());

    for (size_t i = 0; i < activeCount_; ++i) {
        const auto& inst = instances_[i];
        Vector3 vec = inst.end - inst.start;
        float length = Math::Length(vec);
        if (length < 1e-6f) { // ゼロ除算を避ける
            length = 1e-6f;
        }
        Matrix4x4 scale = Math::MakeScaleMatrix({ length, 1.0f, 1.0f });

        Vector3 up = { 0.0f, 1.0f, 0.0f };
        Vector3 dir = Math::Normalize(vec);
        if (abs(Math::Dot(dir, up)) > 0.999f) {
            up = { 1.0f, 0.0f, 0.0f };
        }
        Matrix4x4 rotate = Math::DirectionToDirection({ 1.0f, 0.0f, 0.0f }, dir);

        Matrix4x4 translate = Math::MakeTranslateMatrix(inst.start);
        Matrix4x4 world = scale * rotate * translate;

        instanceData_[i].WVP = world * viewProjection;
        instanceData_[i].color = inst.color;
    }
}

void Line3DRegion::Draw() {
    if (activeCount_ == 0) return;
    BuildInstanceBuffer();
    drawManager_->DrawLineInstanced(baseLineResource_->vertexBufferView_, baseLineResource_->indexBufferView_, instancingSrvGPU_, GetInstanceCountU32());
}

void Line3DRegion::CreateOrResizeInstanceBuffer(uint32_t instanceCount) {
    if (instanceCount == 0) return;
    if (instanceCount > instanceCapacity_) {
        if (instanceBuffer_) {
            instanceBuffer_->Unmap(0, nullptr);
            instanceData_ = nullptr;
            instanceBuffer_.Reset();
        }
        instanceCapacity_ = instanceCount;

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC resDesc{};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width = sizeof(InstanceData) * instanceCapacity_;
        resDesc.Height = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = dx_->GetDevice()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(instanceBuffer_.GetAddressOf()));
        if (FAILED(hr)) {
            instanceBuffer_.Reset();
            instanceCapacity_ = 0;
            return;
        }

        instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&instanceData_));
        EnsureInstancingSRV();
    }
}

void Line3DRegion::EnsureInstancingSRV() {
    if (!instanceBuffer_) return;

    if (instancingSrvIndex_ == UINT32_MAX) {
        instancingSrvIndex_ = s_srvAllocator_->Allocate();
        instancingSrvCPU_ = s_srvAllocator_->GetCPUHandle(instancingSrvIndex_);
        instancingSrvGPU_ = s_srvAllocator_->GetGPUHandle(instancingSrvIndex_);
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = instanceCapacity_;
    srvDesc.Buffer.StructureByteStride = sizeof(InstanceData);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    dx_->GetDevice()->CreateShaderResourceView(instanceBuffer_.Get(), &srvDesc, instancingSrvCPU_);
}