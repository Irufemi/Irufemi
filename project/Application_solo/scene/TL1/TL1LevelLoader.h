#pragma once

#include <string>
#include <nlohmann/json.hpp>

class BaseScene;

/**
 * @class TL1LevelLoader
 * @brief Blenderから出力された独自JSON形式のレベルデータを読み込み、
 *        IrufemiEngine標準のJSON形式にコンバートしてシーンに構築するクラス
 */
class TL1LevelLoader {
public:
    /**
     * @brief レベルデータを読み込み、シーンに配置する
     * @param filepath JSONファイルのパス (例: "resources/configs/TL1.json")
     * @param scene 配置先のシーン
     */
    static void Load(const std::string& filepath, BaseScene* scene);

private:
    /**
     * @brief Blender出力ノードをエンジン標準ノードに変換する
     * @param blenderNode Blender側で出力されたJSONオブジェクト
     * @return 変換後のエンジン標準JSONオブジェクト
     */
    static nlohmann::json ConvertBlenderJsonToEngineJson(const nlohmann::json& blenderNode);
};
