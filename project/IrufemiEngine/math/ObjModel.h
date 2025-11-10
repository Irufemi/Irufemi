#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "VertexData.h"
#include "ModelData.h"
#include "../function/Math.h"
#include <string>
#include <vector>

struct ObjMaterial {
    // Kd
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    // Ka
    Vector3 ambient = { 0.1f, 0.1f, 0.1f };
    // Ks
    Vector3 specular = { 1.0f, 1.0f, 1.0f };
    // Ns
    float shininess = 32.0f;  
    // d
    float alpha = 1.0f;    

    bool enableLighting = true;

    Matrix4x4 uvTransform = Math::MakeIdentity4x4();

    std::string textureFilePath = "";
};

struct ObjMesh {
    std::vector<VertexData> vertices;
    ObjMaterial material;
};

struct ObjModel {
    std::vector<ObjMesh> meshes;
};