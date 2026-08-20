#include "Resource/Model/ModelSerializer.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include "Core/Utility/Log.h"
#include <vector>

namespace {
    template<typename T>
    void WritePOD(std::ofstream& ofs, const T& value) {
        ofs.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    template<typename T>
    void ReadPOD(std::ifstream& ifs, T& value) {
        ifs.read(reinterpret_cast<char*>(&value), sizeof(T));
    }

    void WriteString(std::ofstream& ofs, const std::string& str) {
        uint32_t size = static_cast<uint32_t>(str.size());
        WritePOD(ofs, size);
        if (size > 0) {
            ofs.write(str.data(), size);
        }
    }

    void ReadString(std::ifstream& ifs, std::string& str) {
        uint32_t size = 0;
        ReadPOD(ifs, size);
        if (ifs.fail() || size == 0) {
            str.clear();
            return;
        }

        // 残りファイルサイズによるバリデーション
        auto currentPos = ifs.tellg();
        ifs.seekg(0, std::ios::end);
        auto endPos = ifs.tellg();
        ifs.seekg(currentPos, std::ios::beg);
        
        if (size > static_cast<uint32_t>(endPos - currentPos)) {
            Log::OutPutLog(std::cerr, "[ModelSerializer] Error: String size validation failed.\n");
            str.clear();
            return;
        }

        std::vector<char> buffer(size);
        ifs.read(buffer.data(), size);
        if (ifs.fail()) {
            str.clear();
            return;
        }
        str.assign(buffer.data(), size);
    }

    template<typename T>
    void WriteVectorPOD(std::ofstream& ofs, const std::vector<T>& vec) {
        uint32_t size = static_cast<uint32_t>(vec.size());
        WritePOD(ofs, size);
        if (size > 0) {
            ofs.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
        }
    }

    template<typename T>
    void ReadVectorPOD(std::ifstream& ifs, std::vector<T>& vec) {
        uint32_t size = 0;
        ReadPOD(ifs, size);
        if (ifs.fail() || size == 0) {
            vec.clear();
            return;
        }

        // 残りファイルサイズによるバリデーション
        auto currentPos = ifs.tellg();
        ifs.seekg(0, std::ios::end);
        auto endPos = ifs.tellg();
        ifs.seekg(currentPos, std::ios::beg);
        
        uint64_t bytesNeeded = static_cast<uint64_t>(size) * sizeof(T);
        if (bytesNeeded > static_cast<uint64_t>(endPos - currentPos)) {
            Log::OutPutLog(std::cerr, "[ModelSerializer] Error: Vector size validation failed.\n");
            vec.clear();
            return;
        }

        vec.resize(size);
        ifs.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));
        if (ifs.fail()) {
            vec.clear();
        }
    }

    void WriteMaterial(std::ofstream& ofs, const ObjMaterial& mat) {
        WritePOD(ofs, mat.color);
        WritePOD(ofs, mat.ambient);
        WritePOD(ofs, mat.specular);
        WritePOD(ofs, mat.roughness);
        WritePOD(ofs, mat.metallic);
        WritePOD(ofs, mat.alpha);
        WritePOD(ofs, mat.enableLighting);
        WritePOD(ofs, mat.lightingMode);
        WritePOD(ofs, mat.useClampSampler);
        WritePOD(ofs, mat.environmentCoefficient);
        WritePOD(ofs, mat.alphaReference);
        WritePOD(ofs, mat.uvTransform);
        WriteString(ofs, mat.textureFilePath);
        WriteString(ofs, mat.normalMapFilePath);
    }

    void ReadMaterial(std::ifstream& ifs, ObjMaterial& mat) {
        ReadPOD(ifs, mat.color);
        ReadPOD(ifs, mat.ambient);
        ReadPOD(ifs, mat.specular);
        ReadPOD(ifs, mat.roughness);
        ReadPOD(ifs, mat.metallic);
        ReadPOD(ifs, mat.alpha);
        ReadPOD(ifs, mat.enableLighting);
        ReadPOD(ifs, mat.lightingMode);
        ReadPOD(ifs, mat.useClampSampler);
        ReadPOD(ifs, mat.environmentCoefficient);
        ReadPOD(ifs, mat.alphaReference);
        ReadPOD(ifs, mat.uvTransform);
        ReadString(ifs, mat.textureFilePath);
        ReadString(ifs, mat.normalMapFilePath);
    }

    void WriteNode(std::ofstream& ofs, const Node& node) {
        WritePOD(ofs, node.transform);
        WritePOD(ofs, node.localMatrix);
        WriteString(ofs, node.name);
        
        uint32_t childrenCount = static_cast<uint32_t>(node.children.size());
        WritePOD(ofs, childrenCount);
        for (const auto& child : node.children) {
            WriteNode(ofs, child);
        }
    }

    void ReadNode(std::ifstream& ifs, Node& node) {
        ReadPOD(ifs, node.transform);
        ReadPOD(ifs, node.localMatrix);
        ReadString(ifs, node.name);
        
        uint32_t childrenCount = 0;
        ReadPOD(ifs, childrenCount);
        node.children.resize(childrenCount);
        for (uint32_t i = 0; i < childrenCount; ++i) {
            ReadNode(ifs, node.children[i]);
        }
    }
}

bool ModelSerializer::Serialize(const std::string& filepath, const ObjModel& model, uint64_t sourceLastWriteTime) {
    std::ofstream ofs(filepath, std::ios::binary);
    if (!ofs.is_open()) {
        Log::OutPutLog(std::cerr, "[ModelSerializer] Error: Failed to open file for writing: " + filepath + "\n");
        return false;
    }

    // ヘッダー書き込み
    Header header;
    header.magic = kMagicNumber;
    header.version = kVersion;
    header.sourceLastWriteTime = sourceLastWriteTime;
    WritePOD(ofs, header);

    // メッシュ
    uint32_t meshCount = static_cast<uint32_t>(model.meshes.size());
    WritePOD(ofs, meshCount);
    for (const auto& mesh : model.meshes) {
        WriteVectorPOD(ofs, mesh.vertices);
        WriteVectorPOD(ofs, mesh.indices);
        WriteMaterial(ofs, mesh.material);
        WriteString(ofs, mesh.nodeName);
    }

    // ノードツリー
    WriteNode(ofs, model.rootNode);

    // スキンクラスター
    uint32_t skinCount = static_cast<uint32_t>(model.skinClusterData.size());
    WritePOD(ofs, skinCount);
    for (const auto& pair : model.skinClusterData) {
        WriteString(ofs, pair.first);
        WritePOD(ofs, pair.second.inverseBindPoseMatrix);
        WriteVectorPOD(ofs, pair.second.vertexWeights);
    }

    // 境界ボリューム
    WritePOD(ofs, model.boundingSphere);
    WritePOD(ofs, model.boundingBox);

    return true;
}

bool ModelSerializer::Deserialize(const std::string& filepath, ObjModel& outModel, uint64_t& outSourceLastWriteTime) {
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs.is_open()) {
        Log::OutPutLog(std::cerr, "[ModelSerializer] Error: Failed to open file for reading: " + filepath + "\n");
        return false;
    }

    Header header;
    if (!ReadHeader(filepath, header)) return false; // ヘッダーチェック

    ifs.seekg(sizeof(Header), std::ios::beg); // ヘッダーの次から読み込む

    outSourceLastWriteTime = header.sourceLastWriteTime;

    // メッシュ
    uint32_t meshCount = 0;
    ReadPOD(ifs, meshCount);
    outModel.meshes.resize(meshCount);
    for (uint32_t i = 0; i < meshCount; ++i) {
        ReadVectorPOD(ifs, outModel.meshes[i].vertices);
        ReadVectorPOD(ifs, outModel.meshes[i].indices);
        ReadMaterial(ifs, outModel.meshes[i].material);
        ReadString(ifs, outModel.meshes[i].nodeName);
    }

    // ノードツリー
    ReadNode(ifs, outModel.rootNode);

    // スキンクラスター
    uint32_t skinCount = 0;
    ReadPOD(ifs, skinCount);
    outModel.skinClusterData.clear();
    for (uint32_t i = 0; i < skinCount; ++i) {
        std::string key;
        ReadString(ifs, key);
        JointWeightData data;
        ReadPOD(ifs, data.inverseBindPoseMatrix);
        ReadVectorPOD(ifs, data.vertexWeights);
        outModel.skinClusterData[key] = data;
    }

    // 境界ボリューム
    ReadPOD(ifs, outModel.boundingSphere);
    ReadPOD(ifs, outModel.boundingBox);

    if (ifs.fail()) return false;

    return true;
}

bool ModelSerializer::ReadHeader(const std::string& filepath, Header& outHeader) {
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs.is_open()) {
        Log::OutPutLog(std::cerr, "[ModelSerializer] Error: Failed to open file for reading header: " + filepath + "\n");
        return false;
    }

    ReadPOD(ifs, outHeader);
    if (outHeader.magic != kMagicNumber) return false;
    if (outHeader.version != kVersion) return false;

    return true;
}
