#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <sstream>
#include <cassert>
#include <wrl.h>
#include <d3d12.h>
#include "math/ObjModel.h"
#include "math/ModelData.h"
#include "math/MaterialData.h"
#include "function/Math.h"

// 前方宣言
struct aiNode;
namespace Assimp { class Importer; }
struct aiScene;
struct aiMesh;
struct aiMaterial;
struct Node;
class DirectXCommon;
class TextureManager; // 追加

// 共有されるGPUメッシュリソース
struct GpuMesh {
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView{};
    UINT vertexCount = 0;
    UINT indexCount = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE vertexSrvHandle{}; // 追加
};

// 共有されるGPUマテリアルリソース
struct GpuMaterial {
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle{};
};

// CPU/GPUデータを統合した管理単位
struct ManagedModel {
    std::shared_ptr<ObjModel> cpuModel;
    std::vector<std::shared_ptr<GpuMesh>> gpuMeshes;
    std::vector<std::shared_ptr<GpuMaterial>> gpuMaterials; // 追加
};

class ModelManager {
public:
    ModelManager() = default;
    ~ModelManager() = default;

    // --- インスタンス機能(キャッシュ管理) ---
    void Initialize(DirectXCommon* dxCommon, TextureManager* textureManager); // TextureManager を追加
    void SetRootDirectory(std::string root);
    std::shared_ptr<ManagedModel> GetModel(const std::string& filename);
    void PreloadAllUnder(const std::string& relativeFolder);
    std::vector<std::string> GetCachedKeys() const;
    void CollectGarbage();
    void ClearAll();

    // --- 静的ロード関数 (旧 Function.h 由来) ---
    static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
    static ObjModel LoadObjFileM(const std::string& directoryPath, const std::string& filename);
    static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);
    static ObjModel LoadModelFileM(const std::string& directoryPath, const std::string& filename);
    static bool ParseObjFaceToken(const std::string& token, int& posIdx, int& uvIdx, int& normIdx);
    static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string filename);
    static Node ReadNode(aiNode* node);


private:
    std::string NormalizeAndResolve(const std::string& filename) const;
    static bool StartsWith(const std::string& s, const std::string& prefix);
    static std::pair<std::string, std::string> SplitDirectoryAndFile(const std::string& full);
    void DebugLogLoad(const std::string& key, size_t meshCount);
    std::string FindFileRecursive(const std::string& filename) const;

private:
    DirectXCommon* dxCommon_ = nullptr;
    TextureManager* textureManager_ = nullptr; // 追加
    std::string rootDir_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::weak_ptr<ManagedModel>> cache_;
    mutable std::unordered_map<std::string, std::string> filePathCache_;
};