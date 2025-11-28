#include "LineClass.h"

#include "Application/camera/Camera.h"
#include "manager/DrawManager.h"
#include "engine/directX/DirectXCommon.h"
#include "function/Math.h"

DrawManager* Line2DClass::drawManager_ = nullptr;
DrawManager* Line3DClass::drawManager_ = nullptr;

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

    resource_ = std::make_unique<D3D12ResourceUtilLine>();

    // リソースのメモリを確保
    resource_->CreateResource();

    // 書き込めるようにする
    resource_->Map();

    // 頂点の追加
    resource_->vertexData_[0] = { {origin.x, origin.y,origin.z,1.0f},color };
    resource_->vertexData_[1] = { {end.x, end.y,end.z,1.0f} ,color };

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