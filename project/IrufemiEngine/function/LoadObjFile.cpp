#include "Function.h"

#include "../math/Vector4.h"
#include "../math/Vector3.h"
#include "../math/Vector2.h"
#include "../math/Matrix4x4.h"
#include "../function/Math.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <cassert>
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
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
        } 
        else if (identifier == "vt") {
            Vector2 texcoord;
            s >> texcoord.x >> texcoord.y;
            texcoords.push_back(texcoord);
        } 
        else if (identifier == "vn") {
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

    return modelData;
}


ObjModel LoadObjFileM(const std::string& directoryPath, const std::string& filename) {
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
            // 三角形の回り順は逆にしている（必要な場合のみ）
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
                            { 1.0f, 1.0f, 1.0f }, { 0,0,0 }, { 0,0,0 });
                    }
                }
            }
        }
    }

    if (!currentMesh.vertices.empty()) {
        objModel.meshes.push_back(currentMesh);
    }

    return objModel;
}

ModelData LoadObjFileAssimp(const std::string& directoryPath, const std::string& filename) {

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

    return modelData;

}


ObjModel LoadObjFileAssimpM(const std::string& directoryPath, const std::string& filename) {
    ObjModel objModel;

    Assimp::Importer importer;
    const std::string filePath = directoryPath + "/" + filename;

    // 三角形化 + 回り順反転 + UV反転（左手系化は手動でx *= -1）
    const unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_FlipWindingOrder |
        aiProcess_FlipUVs;

    const aiScene* scene = importer.ReadFile(filePath.c_str(), flags);
    assert(scene && scene->HasMeshes());

    // マテリアルをObjMaterialに変換（テクスチャ/色/不透明度/光沢など）
    std::vector<ObjMaterial> convertedMaterials;
    convertedMaterials.resize(scene->mNumMaterials);

    for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
        const aiMaterial* m = scene->mMaterials[i];
        ObjMaterial out{};

        // デフォルト値
        out.textureFilePath = "";
        out.color = { 1.0f,1.0f,1.0f,1.0f };
        out.ambient = { 0.0f,0.0f,0.0f };
        out.specular = { 0.0f,0.0f,0.0f };
        out.shininess = 32.0f;
        out.alpha = 1.0f;
        out.enableLighting = true;
        out.uvTransform = Math::MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0,0,0 }, { 0,0,0 });

        // テクスチャ（ディフューズ）
        if (m->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texPath;
            if (m->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == aiReturn_SUCCESS) {
                // 埋め込みテクスチャ("*0"など)はここでは未対応。外部ファイルパスのみ対応。
                std::string p = texPath.C_Str();
                if (!p.empty() && p[0] != '*') {
                    // 相対パスをリソースフォルダ基準に解決
                    out.textureFilePath = directoryPath + "/" + p;
                }
            }
        }

        // カラー等（取得できる場合のみ）
        aiColor3D kd;
        if (m->Get(AI_MATKEY_COLOR_DIFFUSE, kd) == aiReturn_SUCCESS) {
            out.color.x = kd.r;
            out.color.y = kd.g;
            out.color.z = kd.b;
            out.color.w = 1.0f;
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
            out.shininess = shininess;
        }
        float opacity = 1.0f;
        if (m->Get(AI_MATKEY_OPACITY, opacity) == aiReturn_SUCCESS) {
            out.alpha = opacity;
            out.color.w = opacity;
        }

        convertedMaterials[i] = out;
    }

    // メッシュを展開（頂点三角形ストリップ → 連続三角形リスト）
    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* mesh = scene->mMeshes[meshIndex];

        ObjMesh outMesh;
        // マテリアル割り当て
        if (mesh->mMaterialIndex < convertedMaterials.size()) {
            outMesh.material = convertedMaterials[mesh->mMaterialIndex];
        }

        // 頂点展開
        assert(mesh->HasFaces());
        // 法線がないモデルもあるため、生成済みでなくても安全に扱う
        const bool hasNormals = mesh->HasNormals();
        const bool hasUV0 = mesh->HasTextureCoords(0);

        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            const aiFace& face = mesh->mFaces[faceIndex];
            assert(face.mNumIndices == 3); // Triangulate済み

            for (uint32_t e = 0; e < 3; ++e) {
                const uint32_t idx = face.mIndices[e];

                const aiVector3D& p = mesh->mVertices[idx];
                aiVector3D n = hasNormals ? mesh->mNormals[idx] : aiVector3D(0, 1, 0);
                aiVector3D t = hasUV0 ? mesh->mTextureCoords[0][idx] : aiVector3D(0.5f, 0.5f, 0.0f);

                VertexData v{};
                // 左手系化（x反転のみ、回り順はフラグで反転済み）
                v.position = { -p.x, p.y, p.z, 1.0f };
                v.normal = { -n.x, n.y, n.z };
                // UVは aiProcess_FlipUVs 済み。追加の反転は不要。
                v.texcoord = { t.x, t.y };

                outMesh.vertices.push_back(v);
            }
        }

        objModel.meshes.push_back(std::move(outMesh));
    }

    return objModel;
}

// f行の頂点データを安全にパースする関数例
bool ParseObjFaceToken(const std::string& token, int& posIdx, int& uvIdx, int& normIdx) {
    posIdx = uvIdx = normIdx = -1; // デフォルト値（0開始なら0に）

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