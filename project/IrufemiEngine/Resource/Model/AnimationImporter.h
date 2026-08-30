#pragma once
#include "Resource/Model/Data/Animation.h"
#include <string>

class AnimationImporter {
public:
    /**
     * @brief Assimpを用いてアニメーションファイル(.gltf)を読み込む
     * @param directoryPath ファイルのあるディレクトリパス
     * @param filename 拡張子を含むファイル名
     * @return 解析済みのAnimation
     */
    static Animation Import(const std::string& fullPath);
};
