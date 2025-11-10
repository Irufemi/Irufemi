#include "Function.h"

#include <fstream>
#include <sstream>
#include <cassert>

/*objファイルを読んでみよう*/

/// MaterialData構造体と読み込み関数

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string filename) {
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