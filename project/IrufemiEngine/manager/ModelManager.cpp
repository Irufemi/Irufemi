#include "ModelManager.h"
#include <filesystem>
#include <Windows.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include "engine/directX/DirectXCommon.h"
#include "manager/TextureManager.h" // 追加
#include "math/Material.h" // Material構造体のため追加

//======================
// キャッシュ系(インスタンス)
//======================

void ModelManager::Initialize(DirectXCommon* dxCommon, TextureManager* textureManager) {
    dxCommon_ = dxCommon;
    textureManager_ = textureManager; // 追加
    if (rootDir_.empty()) {
        rootDir_ = "resources/obj";
    }
}

void ModelManager::SetRootDirectory(std::string root) {
    std::replace(root.begin(), root.end(), '\\', '/');
    if (!root.empty() && root.back() == '/') root.pop_back();
    rootDir_ = std::move(root);
}

std::shared_ptr<ManagedModel> ModelManager::GetModel(const std::string& filename) {
    const std::string key = NormalizeAndResolve(filename);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (auto it = cache_.find(key); it != cache_.end()) {
            if (auto sp = it->second.lock()) {
                return sp;
            }
        }
    }

    // CPUモデルロード
    auto pair = SplitDirectoryAndFile(key);
    auto cpuModel = std::make_shared<ObjModel>(ModelManager::LoadModelFileM(pair.first, pair.second));

    // GPUリソース生成
    auto managedModel = std::make_shared<ManagedModel>();
    managedModel->cpuModel = cpuModel;
    managedModel->gpuMeshes.reserve(cpuModel->meshes.size());
    managedModel->gpuMaterials.reserve(cpuModel->meshes.size()); // 追加

    for (const auto& cpuMesh : cpuModel->meshes) {
        auto gpuMesh = std::make_shared<GpuMesh>();

        // Vertex Buffer
        if (!cpuMesh.vertices.empty()) {
            const size_t vbSize = sizeof(VertexData) * cpuMesh.vertices.size();
            gpuMesh->vertexResource = dxCommon_->CreateBufferResource(vbSize);
            gpuMesh->vertexCount = static_cast<UINT>(cpuMesh.vertices.size());
            gpuMesh->vertexBufferView.BufferLocation = gpuMesh->vertexResource->GetGPUVirtualAddress();
            gpuMesh->vertexBufferView.SizeInBytes = static_cast<UINT>(vbSize);
            gpuMesh->vertexBufferView.StrideInBytes = sizeof(VertexData);
            VertexData* vbData = nullptr;
            gpuMesh->vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vbData));
            std::memcpy(vbData, cpuMesh.vertices.data(), vbSize);
            gpuMesh->vertexResource->Unmap(0, nullptr);
        }

        // Index Buffer (あれば)
        if (!cpuMesh.indices.empty()) {
            const size_t ibSize = sizeof(uint32_t) * cpuMesh.indices.size();
            gpuMesh->indexResource = dxCommon_->CreateBufferResource(ibSize);
            gpuMesh->indexCount = static_cast<UINT>(cpuMesh.indices.size());
            gpuMesh->indexBufferView.BufferLocation = gpuMesh->indexResource->GetGPUVirtualAddress();
            gpuMesh->indexBufferView.SizeInBytes = static_cast<UINT>(ibSize);
            gpuMesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
            uint32_t* ibData = nullptr;
            gpuMesh->indexResource->Map(0, nullptr, reinterpret_cast<void**>(&ibData));
            std::memcpy(ibData, cpuMesh.indices.data(), ibSize);
            gpuMesh->indexResource->Unmap(0, nullptr);
        }
        managedModel->gpuMeshes.push_back(std::move(gpuMesh));

        // Materialリソース生成
        auto gpuMaterial = std::make_shared<GpuMaterial>();
        gpuMaterial->materialResource = dxCommon_->CreateBufferResource(sizeof(Material));
        Material* materialData = nullptr;
        gpuMaterial->materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

        materialData->color = cpuMesh.material.color;
        materialData->enableLighting = cpuMesh.material.enableLighting;
        materialData->uvTransform = cpuMesh.material.uvTransform;
        materialData->shininess = cpuMesh.material.shininess;
        materialData->hasTexture = !cpuMesh.material.textureFilePath.empty();
        materialData->lightingMode = cpuMesh.material.enableLighting ? 2 : 0;
        if (materialData->color.w <= 0.0f) { materialData->color.w = 1.0f; }

        // テクスチャハンドル取得
        if (materialData->hasTexture) {
            gpuMaterial->textureHandle = textureManager_->GetTextureHandle(cpuMesh.material.textureFilePath);
        } else {
            gpuMaterial->textureHandle = textureManager_->GetTextureHandle("resources/whiteTexture.png");
        }
        managedModel->gpuMaterials.push_back(std::move(gpuMaterial));
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_[key] = managedModel;
    }

    DebugLogLoad(key, managedModel->cpuModel->meshes.size());
    return managedModel;
}

void ModelManager::PreloadAllUnder(const std::string& relativeFolder) {
    namespace fs = std::filesystem;
    const std::string rootBase = rootDir_.empty() ? "resources/obj" : rootDir_;
    fs::path start = fs::path(rootBase) / relativeFolder;
    if (!fs::exists(start)) { return; }

    for (auto& entry : fs::recursive_directory_iterator(start)) {
        if (!entry.is_regular_file()) continue;
        auto p = entry.path();
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext == ".obj" || ext == ".gltf" || ext == ".glb") {
            GetModel(p.string());
        }
    }
}

std::vector<std::string> ModelManager::GetCachedKeys() const {
    std::vector<std::string> out;
    std::lock_guard<std::mutex> lock(mutex_);
    out.reserve(cache_.size());
    for (auto& kv : cache_) {
        if (!kv.second.expired()) {
            out.push_back(kv.first);
        }
    }
    return out;
}

void ModelManager::CollectGarbage() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = cache_.begin(); it != cache_.end();) {
        if (it->second.expired()) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

void ModelManager::ClearAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
}

std::string ModelManager::NormalizeAndResolve(const std::string& filename) const {
    std::string f = filename;
    std::replace(f.begin(), f.end(), '\\', '/');
    if (StartsWith(f, rootDir_ + "/")) {
        // OK
    } else if (StartsWith(f, rootDir_)) {
        f = rootDir_ + "/" + f.substr(rootDir_.size());
    } else {
        f = rootDir_ + "/" + f;
    }
    std::transform(f.begin(), f.end(), f.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return f;
}

bool ModelManager::StartsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() &&
        std::equal(prefix.begin(), prefix.end(), s.begin());
}

std::pair<std::string, std::string> ModelManager::SplitDirectoryAndFile(const std::string& full) {
    auto pos = full.find_last_of('/');
    if (pos == std::string::npos) return { ".", full };
    return { full.substr(0, pos), full.substr(pos + 1) };
}

void ModelManager::DebugLogLoad(const std::string& key, size_t meshCount) {
#if defined(_DEBUG) || defined(DEVELOPMENT)
    std::string msg = "[ModelManager] Loaded GPU resources for: " + key +
        " meshes=" + std::to_string(meshCount) + "\n";
    OutputDebugStringA(msg.c_str());
#endif
}

//======================
// 静的ロード関数群(旧 Function.h 移植)
//======================

MaterialData ModelManager::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string filename) {
    // 1. 中で必要となる変数の宣言
    // 2. ファイルを開く
    // 3. 実際にファイルを読み、MaterialDataを構築していく
    // 4. MaterialDataを返す

    ///1.2. 必要な宣言とファイルを開く

    MaterialData materialData;
    std::string line; //ファイルから読んだ1行を格納するもの
    std::ifstream file(directoryPath + "/" + filename); //ファイルを開く
    assert(file.is_open()); //とりあえず開けなかったら止める

    ///3. ファイルを読み、MaterialDataを構築

    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        // identifierに応じた処理
        if (identifier == "map_Kd") {
            std::string textureFilename;
            s >> textureFilename;
            //連結してファイルパスにする
            materialData.textureFilePath = directoryPath + "/" + textureFilename;
        }
    }
    return materialData;
}

ModelData ModelManager::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
    // 1. 中で必要となる変数の宣言
    // 2. ファイルを開く
    // 3. 実際にファイルを読み、ModelDataを構築していく
    // 4. ModelDataを返す

    /// 1.2.必要な変数の宣言とファイルを開く

    ModelData modelData; //構築するModelData
    std::vector<Vector4> positions; //位置
    std::vector<Vector3> normals; //法線
    std::vector<Vector2> texcoords; //テクスチャ座標
    std::string line; //ファイルから読んだ1行を格納するもの

    std::ifstream file(directoryPath + "/" + filename); //ファイルを開く
    assert(file.is_open()); //とりあえず開けなかったら止める

    ///3.ファイルを読み、ModelDataを構築
    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier; //先頭の識別子を読む


        //identifierに応じた処理

        ///頂点情報を読む
        if (identifier == "v") {
            Vector4 position;
            s >> position.x >> position.y >> position.z;
            position.w = 1.0f;
            positions.push_back(position);
        } else if (identifier == "vt") {
            Vector2 texcoord;
            s >> texcoord.x >> texcoord.y;
            texcoords.push_back(texcoord);
        } else if (identifier == "vn") {
            Vector3 normal;
            s >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }

        ///三角形を作る

        else if (identifier == "f") {
            VertexData triangle[3];
            //面は三角形限定。その他は未対応
            for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
                std::string vertexDefinition;
                s >> vertexDefinition;
                //頂点の要素のIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
                std::istringstream v(vertexDefinition);
                uint32_t elementIndices[3];
                for (int32_t element = 0; element < 3; ++element) {
                    std::string index;
                    std::getline(v, index, '/'); //区切りでインデックスを読んでいく
                    elementIndices[element] = std::stoi(index);
                }
                //要素へのIndexから、実際の要素の値を取得して、頂点を構築する
                Vector4 position = positions[elementIndices[0] - 1];
                Vector2 texcoord = texcoords[elementIndices[1] - 1];
                Vector3 normal = normals[elementIndices[2] - 1];
                //VertexData vertex = { position,texcoord,normal };
                //modelData.vertices.push_back(vertex);

                ///右手系から左手系へ

                position.x *= -1.0f;
                normal.x *= -1.0f;

                ///Texture座標の原点

                texcoord.y = 1.0f - texcoord.y;

                ///右手系から左手系へ

                triangle[faceVertex] = { position,texcoord,normal };

            }

            //頂点を逆順で登録することで、回り順を逆にする
            modelData.vertices.push_back(triangle[2]);
            modelData.vertices.push_back(triangle[1]);
            modelData.vertices.push_back(triangle[0]);
        }

        ///obj読み込みにmaterial読み込みを追加

        else if (identifier == "mtllib") {
            //materialTempalateLibraryファイルの名前を取得する
            std::string materialFilename;
            s >> materialFilename;
            //基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイルを渡す
            modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
        }
    }

    modelData.rootNode = Node{};

    return modelData;
}

// f行の頂点データを安全にパースする関数例
bool ModelManager::ParseObjFaceToken(const std::string& token, int& posIdx, int& uvIdx, int& normIdx) {
    posIdx = uvIdx = normIdx = -1; // デフォルト値(0開始なら0に)

    size_t firstSlash = token.find('/');
    size_t secondSlash = token.find('/', firstSlash + 1);

    // 位置インデックス
    if (firstSlash == std::string::npos) {
        // 例: "1"
        if (!token.empty()) posIdx = std::stoi(token);
    } else {
        // 例: "1/2/3", "1//3", "1/2"
        if (firstSlash > 0) posIdx = std::stoi(token.substr(0, firstSlash));
        // UVインデックス
        if (secondSlash != std::string::npos) {
            // "1/2/3"
            if (secondSlash > firstSlash + 1) uvIdx = std::stoi(token.substr(firstSlash + 1, secondSlash - firstSlash - 1));
            // 法線インデックス
            if (token.size() > secondSlash + 1) normIdx = std::stoi(token.substr(secondSlash + 1));
        } else {
            // "1/2"
            if (token.size() > firstSlash + 1) uvIdx = std::stoi(token.substr(firstSlash + 1));
        }
    }
    return true;
}

ObjModel ModelManager::LoadObjFileM(const std::string& directoryPath, const std::string& filename) {
    ObjModel objModel;
    std::vector<Vector4> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::map<std::string, ObjMaterial> materialMap;

    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());

    std::string line;
    ObjMesh currentMesh;

    while (std::getline(file, line)) {
        std::istringstream s(line);
        std::string id;
        s >> id;

        if (id == "v") {
            Vector4 pos;
            s >> pos.x >> pos.y >> pos.z;
            pos.w = 1.0f;
            // 左手系変換はここだけ
            pos.x *= -1.0f;
            positions.push_back(pos);
        } else if (id == "vt") {
            Vector2 uv;
            s >> uv.x >> uv.y;
            // y反転のみここで
            uv.y = 1.0f - uv.y;
            texcoords.push_back(uv);
        } else if (id == "vn") {
            Vector3 n;
            s >> n.x >> n.y >> n.z;
            // 左手系変換はここだけ
            n.x *= -1.0f;
            normals.push_back(n);
        } else if (id == "f") {
            VertexData tri[3];
            for (int i = 0; i < 3; ++i) {
                std::string def;
                s >> def;
                int pIdx = -1, tIdx = -1, nIdx = -1;
                ParseObjFaceToken(def, pIdx, tIdx, nIdx);

                Vector4 position = (pIdx > 0) ? positions[pIdx - 1] : Vector4{};
                Vector2 texcoord = (tIdx > 0) ? texcoords[tIdx - 1] : Vector2{ 0.5f, 0.5f };
                Vector3 normal = (nIdx > 0) ? normals[nIdx - 1] : Vector3{};

                tri[i] = { position, texcoord, normal };
            }
            // 三角形の回り順は逆にしている(必要な場合のみ)
            currentMesh.vertices.push_back(tri[2]);
            currentMesh.vertices.push_back(tri[1]);
            currentMesh.vertices.push_back(tri[0]);
        } else if (id == "usemtl") {
            if (!currentMesh.vertices.empty()) {
                objModel.meshes.push_back(currentMesh);
                currentMesh = ObjMesh();
            }
            std::string matName;
            s >> matName;
            if (materialMap.count(matName)) {
                currentMesh.material = materialMap[matName];
            } else {
                currentMesh.material = ObjMaterial(); // デフォルト値
            }
        } else if (id == "mtllib") {
            std::string mtlFilename;
            s >> mtlFilename;
            std::ifstream mtlFile(directoryPath + "/" + mtlFilename);
            assert(mtlFile.is_open());

            std::string mtlLine, currentName;
            while (std::getline(mtlFile, mtlLine)) {
                std::istringstream ms(mtlLine);
                std::string mtlId;
                ms >> mtlId;

                if (mtlId == "newmtl") {
                    ms >> currentName;
                    materialMap[currentName] = ObjMaterial();
                } else if (mtlId == "Kd") {
                    ms >> materialMap[currentName].color.x
                        >> materialMap[currentName].color.y
                        >> materialMap[currentName].color.z;
                    materialMap[currentName].color.w = 1.0f;
                } else if (mtlId == "Ka") {
                    ms >> materialMap[currentName].ambient.x
                        >> materialMap[currentName].ambient.y
                        >> materialMap[currentName].ambient.z;
                } else if (mtlId == "Ks") {
                    ms >> materialMap[currentName].specular.x
                        >> materialMap[currentName].specular.y
                        >> materialMap[currentName].specular.z;
                } else if (mtlId == "Ns") {
                    ms >> materialMap[currentName].shininess;
                } else if (mtlId == "d" || mtlId == "Tr") {
                    ms >> materialMap[currentName].alpha;
                } else if (mtlId == "map_Kd") {
                    std::string token;
                    bool hasTransform = false;
                    // テクスチャオプション対応
                    while (ms >> token) {
                        if (token == "-o") {
                            ms >> materialMap[currentName].uvTransform.m[3][0]
                                >> materialMap[currentName].uvTransform.m[3][1];
                            hasTransform = true;
                        } else if (token == "-s") {
                            ms >> materialMap[currentName].uvTransform.m[0][0]
                                >> materialMap[currentName].uvTransform.m[1][1];
                            hasTransform = true;
                        } else {
                            materialMap[currentName].textureFilePath = directoryPath + "/" + token;
                            break;
                        }
                    }
                    // デフォルト値セット
                    if (!hasTransform) {
                        materialMap[currentName].uvTransform = Math::MakeAffineMatrix(
                            { 1.0f, 1.0f, 1.0f }, Vector3{ 0,0,0 }, { 0,0,0 });
                    }
                }
            }
        }
    }

    if (!currentMesh.vertices.empty()) {
        objModel.meshes.push_back(currentMesh);
    }

    // 手書きパーサでは階層情報はないため空 Node
    objModel.rootNode = Node{};

    return objModel;
}

ModelData ModelManager::LoadModelFile(const std::string& directoryPath, const std::string& filename) {

    ModelData modelData; //構築するModelData

    /*いろんなフォーマットのモデルが読みたい*/

    /// assimpでobjを読む

    // ファイルからassimpのSceneを構築する
    // assimpのデータ構造 → https://learnopengl.com/Model-Loading/Assimp
    Assimp::Importer importer;
    std::string filePath = directoryPath + "/" + filename;
    // assimpでは読み込む際にオプションを指定することができる
    // 今回はobjからDirectX12の形式に合わせるために
    // ・ aiProcess_FlipWindingOrder : 三角形の並び順を逆にする
    // ・ aiProcess_FlipUVs : UVをフリップする(texcoord.y = 1.0f - texcoord.y;の処理)
    // を指定した。
    // ほかのオプション → https://github.com/assimp/assimp/blob/master/include/assimp/postprocess.h#L60
    const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);

    /// meshを解析する

    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        assert(mesh->HasNormals()); // 法線がないMeshは今回は非対応
        assert(mesh->HasTextureCoords(0)); // TexcoordがないMeshは今回は非対応
        // ここからMeshの中身(Face)の解析を行っていく

        /// faceを解析する

        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            aiFace& face = mesh->mFaces[faceIndex];
            assert(face.mNumIndices == 3); // 三角形のみサポート
            // ここからfaceの中身(Vertex)の解析を行っていく

            /// vertexを解析する
            for (uint32_t element = 0; element < face.mNumIndices; ++element) {
                uint32_t vertexIndex = face.mIndices[element];
                aiVector3D& position = mesh->mVertices[vertexIndex];
                aiVector3D& normal = mesh->mNormals[vertexIndex];
                aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
                VertexData vertex;
                vertex.position = { position.x,position.y,position.z };
                vertex.normal = { normal.x,normal.y,normal.z };
                vertex.texcoord = { texcoord.x,texcoord.y };
                // aiProcess_MakeLeftHandedはz*=-1で、右手->左手に変換するので手動で対処
                vertex.position.x *= -1.0f;
                vertex.normal.x *= -1.0f;
                modelData.vertices.push_back(vertex);
            }
        }
    }

    /// materialを解析する

    for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
        aiMaterial* material = scene->mMaterials[materialIndex];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
            aiString textureFilePath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
            modelData.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
        }
    }

    /*glTFを読み込んでみよう*/

    /// assimpでNodを解析する

    modelData.rootNode = ReadNode(scene->mRootNode);

    return modelData;

}

// ObjModel Node 対応 Assimp 版
ObjModel ModelManager::LoadModelFileM(const std::string& directoryPath, const std::string& filename) {
    ObjModel objModel;

    /* いろんなフォーマットのモデルが読みたい */

    /// assimpでobj(glTF等も含む汎用)を読む

    Assimp::Importer importer;
    const std::string filePath = directoryPath + "/" + filename;

    // 読み込み時オプション:
    // ・ aiProcess_Triangulate        : 非三角形ポリゴンを三角化
    // ・ aiProcess_FlipWindingOrder  : 三角形の並び順を逆にして表裏判定を左手系用に合わせる
    // ・ aiProcess_FlipUVs           : UVのV(y)成分を反転
    // ・ aiProcess_MakeLeftHanded    : 右手座標系から左手座標系へ変換(Z反転、行列の調整など全て行う)
    const unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_FlipWindingOrder |
        aiProcess_FlipUVs |
        aiProcess_MakeLeftHanded; // このフラグを追加

    const aiScene* scene = importer.ReadFile(filePath.c_str(), flags);
    assert(scene && scene->HasMeshes()); // 失敗したらsceneはnullptr / メッシュが無い場合は非対応

    /// material(assimpのaiMaterial)をObjMaterialへ変換

    std::vector<ObjMaterial> convertedMaterials(scene->mNumMaterials);

    for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
        const aiMaterial* m = scene->mMaterials[i];
        ObjMaterial out{};

        // デフォルト初期化 (※ 読み込めなかったパラメータを安全値で埋める)
        out.textureFilePath = "";
        out.color     = { 1.0f,1.0f,1.0f,1.0f };
        out.ambient   = { 0.0f,0.0f,0.0f };
        out.specular  = { 0.0f,0.0f,0.0f };
        out.shininess = 64.0f;
        out.alpha     = 1.0f;
        out.enableLighting = true;
        out.uvTransform = Math::MakeAffineMatrix({ 1.0f,1.0f,1.0f }, Vector3{ 0,0,0 }, { 0,0,0 });

        // Diffuse テクスチャ (埋め込み "*0" 等は今回は未対応)
        if (m->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texPath;
            if (m->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == aiReturn_SUCCESS) {
                std::string p = texPath.C_Str();
                if (!p.empty() && p[0] != '*') {
                    out.textureFilePath = directoryPath + "/" + p; // 相対パスを呼び出し元ディレクトリ基準で連結
                }
            }
        }

        // 色/光沢/不透明度 (取得できたもののみ上書き)
        aiColor3D kd;
        if (m->Get(AI_MATKEY_COLOR_DIFFUSE, kd) == aiReturn_SUCCESS) {
            out.color.x = kd.r; out.color.y = kd.g; out.color.z = kd.b; out.color.w = 1.0f;
        }
        aiColor3D ka;
        if (m->Get(AI_MATKEY_COLOR_AMBIENT, ka) == aiReturn_SUCCESS) {
            out.ambient.x = ka.r; out.ambient.y = ka.g; out.ambient.z = ka.b;
        }
        aiColor3D ks;
        if (m->Get(AI_MATKEY_COLOR_SPECULAR, ks) == aiReturn_SUCCESS) {
            out.specular.x = ks.r; out.specular.y = ks.g; out.specular.z = ks.b;
        }
        float shininess = 0.0f;
        if (m->Get(AI_MATKEY_SHININESS, shininess) == aiReturn_SUCCESS) {
            out.shininess = shininess > 0.0f ? shininess : 64.0f;
        }
        float opacity = 1.0f;
        if (m->Get(AI_MATKEY_OPACITY, opacity) == aiReturn_SUCCESS) {
            out.alpha = opacity;
            out.color.w = opacity;
        }

        convertedMaterials[i] = out;
    }

    /// mesh(aiMesh)を解析し ObjMesh を構築

    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        ObjMesh outMesh;

        // マテリアル割り当て (安全に index 範囲内か確認)
        if (mesh->mMaterialIndex < convertedMaterials.size()) {
            outMesh.material = convertedMaterials[mesh->mMaterialIndex];
        }

        // 頂点データの読み込み
        outMesh.vertices.resize(mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
            const aiVector3D& p = mesh->mVertices[i];
            const aiVector3D& n = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0, 1, 0);
            const aiVector3D& t = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.5f, 0.5f, 0);

            VertexData& v = outMesh.vertices[i];
            // Assimpが変換してくれるので、手動での反転は不要になる
            v.position = { p.x, p.y, p.z, 1.0f };
            v.normal = { n.x, n.y, n.z };
            v.texcoord = { t.x, t.y };
        }

        // インデックスデータの読み込み
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            const aiFace& face = mesh->mFaces[faceIndex];
            assert(face.mNumIndices == 3);
            outMesh.indices.push_back(face.mIndices[0]);
            outMesh.indices.push_back(face.mIndices[1]);
            outMesh.indices.push_back(face.mIndices[2]);
        }

        objModel.meshes.push_back(std::move(outMesh));
    }

    /// Node 階層(structure)を解析 (シーンルートから再帰構築)

    objModel.rootNode = ReadNode(scene->mRootNode);

    return objModel;
}


/*glTFを読み込んでみよう*/

/// 前準備

Node ModelManager::ReadNode(aiNode* node) {

    /// assimpでNodを解析する

    Node result;

    /*Skeleton*/

    /// Nodeを拡張する

    aiVector3D scale, translate;
    aiQuaternion rotate;
    node->mTransformation.Decompose(scale, rotate, translate); // assimpの行列からSRTを抽出する関数を利用
    result.transform.scale = { scale.x, scale.y, scale.z }; // Scaleはそのまま
    result.transform.rotate = { rotate.x, rotate.y, rotate.z, rotate.w }; // x軸を反転、さらに回転方向が逆なので軸を反転させる
    result.transform.translate = { -translate.x, translate.y, translate.z }; // x軸を反転
    result.localMatrix = Math::MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

    /*glTFを読み込んでみよう*/

    /// 前準備

    aiMatrix4x4 aiLocalMatrix = node->mTransformation; // nodeのlocalMatrixを取得
    aiLocalMatrix.Transpose(); // 列ベクトル形式を行ベクトル形式に転置
    //result.localMatrix.m[0][0] = aiLocalMatrix[0][0]; // 他の要素も同様に
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            result.localMatrix.m[r][c] = aiLocalMatrix[r][c];
        }
    }

    result.name = node->mName.C_Str(); // Nodeを格納
    result.children.resize(node->mNumChildren); // 子供の数だけ確保
    for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
        // 再帰的に読んで階層構造を作っていく
        result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
    }
    return result;
}