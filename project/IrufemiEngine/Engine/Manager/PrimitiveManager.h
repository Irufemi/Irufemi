#pragma once

#include "math/VertexData.h"
#include "math/PrimitiveType.h"
#include <cstdint>
#include <vector>

class PrimitiveManager
{
public: // メンバ関数
    // 指定した形状のプリミティブを取得する
    static void GetPrimitive(const PrimitiveType& primitiveType, std::vector<VertexData>& vertexDataList, std::vector<uint32_t>& indexDataList);

};

