#pragma once
#include "Resource/Model/Data/ObjModel.h"
#include <string>

class ModelImporter {
public:
    /**
     * @brief Assimpを用いて汎用モデルファイル(.obj, .gltf)を読み込む
     * @param directoryPath ファイルのあるディレクトリパス
     * @param filename 拡張子を含むファイル名
     * @return 解析済みのObjModel
     */
    static ObjModel Import(const std::string& fullPath);
};
