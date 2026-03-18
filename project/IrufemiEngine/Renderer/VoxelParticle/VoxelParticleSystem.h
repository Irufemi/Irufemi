#pragma once
#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector3Int.h"
#include "Engine/Core/Math/Vector4.h"
#include "Resource/Model/Data/VoxelizedModel.h"
#include "Engine/Core/Type/PerView.h"
#include <d3d12.h>
#include <memory>
#include <string>
#include <wrl.h>


// 前方宣言
class IrufemiEngine;
class Camera;
class ModelManager;
class TextureManager;
struct OBB;

// HLSL側のVoxelParticle構造体と一致させる
struct VoxelParticle {
  Vector3 position;
  float life;
  Vector3 velocity;
  float size;
  Vector4 color;
  Vector3 normal;
  uint32_t isActive; // 0:非アクティブ, 1:アクティブ
};

// HLSL側のVoxelEmitter構造体と一致させる（16バイトアライメント対応 = 80バイト）
struct VoxelEmitter {
  Vector3 emitPosition = {0.0f, 0.0f, 0.0f};
  float time = 0.0f;
  float lifeTime = 0.8f; // 短くすることで「はじける」感を出し、残像を防ぐ
  float gravity = 2.0f; // 重力を弱める
  uint32_t emit = 0;
  float dispersion = 8.0f; // 拡散を強める
  float convergence = 0.1f; // 収束を弱める
  Vector3 baseVelocity = {0.0f, 0.0f, 0.0f};
  Vector3 rotate = {0.0f, 0.0f, 0.0f};
  float pad1 = 0.0f;
  Vector3 scale = {1.0f, 1.0f, 1.0f};
  float pad0 = 0.0f;

  // 衝突判定用 (OBB近似)
  Vector3 collisionCenter;
  uint32_t useCollision = 0; // 0:無効, 1:有効
  Vector4 collisionOrientations[3]; // 配列要素は16バイトアラインメントが必要
  Vector3 collisionSize;
  float pad2 = 0.0f; // 16バイトアライメント調整用
};


class VoxelParticleSystem {
public:
  VoxelParticleSystem() = default;
  ~VoxelParticleSystem() = default;

  static void SetEngine(IrufemiEngine *engine) { engine_ = engine; }

  void Initialize(const std::string &modelName, const Vector3Int &resolution,
                  Camera *camera);

  void Update(float deltaTime);
  void Draw();
  void Debug(const char *name);

  void Emit(const Vector3 &position);
  void Explode(const Vector3 &position, const Vector3 &velocity,
               const Vector3 &rotate, const Vector3 &scale);
  
  // 指定したOBBの範囲内にあるボクセルのみをはじけさせる
  void CollisionScatter(const Vector3& position, const Vector3& velocity,
                        const Vector3& rotate, const Vector3& scale,
                        const struct OBB& collisionArea);

  bool IsActive() const {
    if (!hasExploded_)
      return false;
    // 爆散（hasExploded_ = true, time = 0）から lifeTime (+余裕) が経過するまではアクティブ
    return emitterData_.time < (emitterData_.lifeTime + 2.0f);
  }

private:
  void CreateResources();
  void CreatePSO();
  void CreateCubeMesh(float sizeX, float sizeY, float sizeZ);

private:
  Camera *camera_ = nullptr;
  ModelManager *modelManager_ = nullptr;
  TextureManager *textureManager_ = nullptr;
  ID3D12Device *device_ = nullptr;

  std::unique_ptr<VoxelizedModel> voxelModel_;

  // GPUリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> voxelBuffer_;
  Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer_;
  Microsoft::WRL::ComPtr<ID3D12Resource> emitterConstantBuffer_;
  Microsoft::WRL::ComPtr<ID3D12Resource> perViewConstantBuffer_;
  Microsoft::WRL::ComPtr<ID3D12Resource> perFrameConstantBuffer_;
  Microsoft::WRL::ComPtr<ID3D12Resource> cubeVertexBuffer_;
  Microsoft::WRL::ComPtr<ID3D12Resource> cubeIndexBuffer_;

  // デスクリプタハンドル
  D3D12_CPU_DESCRIPTOR_HANDLE voxelSrvHandleCPU_{};
  D3D12_GPU_DESCRIPTOR_HANDLE voxelSrvHandleGPU_{};
  D3D12_CPU_DESCRIPTOR_HANDLE particleSrvHandleCPU_{};
  D3D12_GPU_DESCRIPTOR_HANDLE particleSrvHandleGPU_{};
  D3D12_CPU_DESCRIPTOR_HANDLE particleUavHandleCPU_{};
  D3D12_GPU_DESCRIPTOR_HANDLE particleUavHandleGPU_{};

  // メッシュビュー
  D3D12_VERTEX_BUFFER_VIEW cubeVertexBufferView_{};
  D3D12_INDEX_BUFFER_VIEW cubeIndexBufferView_{};
  uint32_t cubeIndexCount_ = 0;

  // PSO
  Microsoft::WRL::ComPtr<ID3D12PipelineState> initializePSO_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> updatePSO_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> emitPSO_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> drawPSO_;

  VoxelEmitter emitterData_{};
  VoxelEmitter *mappedEmitterData_ = nullptr;
  PerView *mappedPerViewData_ = nullptr;
  struct PerFrame { float time; float deltaTime; };
  PerFrame *mappedPerFrameData_ = nullptr;
  PerFrame perFrameData_{};

  uint32_t voxelCount_ = 0;
  bool isEmitting_ = false;
  bool hasExploded_ = false;
  bool needsInitialize_ = true;
  uint32_t debugFrameCount_ = 0; // デバッグログ用

  static IrufemiEngine *engine_;
};
