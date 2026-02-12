#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "VertexData.h"
#include "ModelData.h"
#include "Node.h"              // 追加: 階層構造を扱うため
#include "function/Math.h"
#include "JointWeightData.h"
#include <string>
#include <vector>
#include <map>

struct ObjMaterial {
    // Kd
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    // Ka
    Vector3 ambient = { 0.1f, 0.1f, 0.1f };
    // Ks
    Vector3 specular = { 1.0f, 1.0f, 1.0f };
    // Ns
    float shininess = 64.0f;  
    // d
    float alpha = 1.0f;    

    bool enableLighting = true;

    // 環境マップの映り込み係数
    float environmentCoefficient = 1.0f;

    Matrix4x4 uvTransform = Math::MakeIdentity4x4();

    std::string textureFilePath = "";
};

struct ObjMesh {

    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices; // 追加
    ObjMaterial material;
    std::string nodeName; // 追加: このメッシュが属するノード名
};

// 階層(Node)を統合した拡張版 ObjModel
struct ObjModel {
    std::vector<ObjMesh> meshes;
    Node rootNode; // 追加: シーン階層ルート
    std::map<std::string, JointWeightData> skinClusterData;
};