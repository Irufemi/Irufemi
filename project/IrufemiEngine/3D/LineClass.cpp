#include "LineClass.h"

#include "Application/camera/Camera.h"
#include "manager/DrawManager.h"
#include "engine/directX/DirectXCommon.h"
#include "function/Math.h"
#include "engine/directX/DescriptorPool.h"

DrawManager* Line2DClass::drawManager_ = nullptr;
DrawManager* Line3DClass::drawManager_ = nullptr;
DirectXCommon* Line3DRegion::dx_ = nullptr;
DrawManager* Line3DRegion::drawManager_ = nullptr;
DescriptorPool* Line3DRegion::s_srvAllocator_ = nullptr;

// コンストラクタ
Line2DClass::Line2DClass() = default;

// デストラクタ
Line2DClass::~Line2DClass() = default;

// 初期化
void Line2DClass::Initialize(Camera* camera, const Vector2& origin, const Vector2& end) {

    camera_ = camera;

    origin_ = origin;
    end_ = end;

    resource_ = std::make_unique<D3D12ResourceUtilLine>();

    // リソースのメモリを確保
    resource_->CreateResource();

    // 書き込めるようにする
    resource_->Map();

    // 頂点の追加
    resource_->vertexData_[0] = { { origin.x, origin.y,0.0f,1.0f },color_ };
    resource_->vertexData_[1] = { { end.x, end.y,0.0f,1.0f },color_ };

    // indexの割り当て
    resource_->indexData_[0] = 0;
    resource_->indexData_[1] = 1;

    // 頂点バッファビュー
    resource_->vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};
    resource_->vertexBufferView_.BufferLocation = resource_->vertexResource_->GetGPUVirtualAddress();
    resource_->vertexBufferView_.StrideInBytes = sizeof(LineVertexData);
    resource_->vertexBufferView_.SizeInBytes = sizeof(LineVertexData) * static_cast<UINT>(2);

    // インデックスバッファビュー
    resource_->indexBufferView_ = D3D12_INDEX_BUFFER_VIEW{};
    resource_->indexBufferView_.BufferLocation = resource_->indexResource_->GetGPUVirtualAddress();
    resource_->indexBufferView_.SizeInBytes = sizeof(uint32_t) * static_cast<UINT>(2);
    resource_->indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    // マテリアル
    resource_->materialData_->color = { 1.0f,1.0f,1.0f,1.0f };

    // WVP計算
    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, camera_->GetOrthographicMatrix());
    *resource_->transformationData_ = { resource_->transformationMatrix_.WVP,resource_->transformationMatrix_.world };
}

// 更新
void Line2DClass::Update() {

    // WVP計算
    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, camera_->GetOrthographicMatrix());
    *resource_->transformationData_ = { resource_->transformationMatrix_.WVP,resource_->transformationMatrix_.world };

}

// 描画
void Line2DClass::Draw() {
    drawManager_->DrawLine2D(this);

}

// コンストラクタ
Line3DClass::Line3DClass() = default;

// デストラクタ
Line3DClass::~Line3DClass() = default;


void Line3DClass::Initialize(Camera* camera, const Vector3& origin, const Vector3& end, const Vector4& color) {

    camera_ = camera;

    origin_ = origin;
    end_ = end;
    color_ = color;

    resource_ = std::make_unique<D3D12ResourceUtilLine>();

    // リソースのメモリを確保
    resource_->CreateResource();

    // 書き込めるようにする
    resource_->Map();

    // 頂点の追加
    resource_->vertexData_[0] = { {origin.x, origin.y,origin.z,1.0f},color_ };
    resource_->vertexData_[1] = { {end.x, end.y,end.z,1.0f} ,color_ };

    // indexの割り当て
    resource_->indexData_[0] = 0;
    resource_->indexData_[1] = 1;

    // 頂点バッファビュー
    resource_->vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};
    resource_->vertexBufferView_.BufferLocation = resource_->vertexResource_->GetGPUVirtualAddress();
    resource_->vertexBufferView_.StrideInBytes = sizeof(LineVertexData);
    resource_->vertexBufferView_.SizeInBytes = sizeof(LineVertexData) * static_cast<UINT>(2);

    // インデックスバッファビュー
    resource_->indexBufferView_ = D3D12_INDEX_BUFFER_VIEW{};
    resource_->indexBufferView_.BufferLocation = resource_->indexResource_->GetGPUVirtualAddress();
    resource_->indexBufferView_.SizeInBytes = sizeof(uint32_t) * static_cast<UINT>(2);
    resource_->indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    // マテリアル
    resource_->materialData_->color = { 1.0f,1.0f,1.0f,1.0f };

    // WVP計算
    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));
    *resource_->transformationData_ = { resource_->transformationMatrix_.WVP,resource_->transformationMatrix_.world };
}

// 更新
void Line3DClass::Update() {

    // WVP計算
    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));
    *resource_->transformationData_ = { resource_->transformationMatrix_.WVP,resource_->transformationMatrix_.world };
}

// 描画
void Line3DClass::Draw() {
    drawManager_->DrawLine3D(this);
}

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
    drawManager_->DrawLine3DRegion(this);
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