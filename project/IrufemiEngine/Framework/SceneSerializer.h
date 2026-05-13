#pragma once
#include <string>

class IScene;

/**
 * @class SceneSerializer
 * @brief シーン内の GameObject 群を JSON 形式で保存・読み込みするクラス
 */
class SceneSerializer {
public:
    /**
     * @brief シーンの状態をファイルに保存する
     * @param scene 保存対象のシーン
     * @param sceneName シーン名（ファイル名として使用）
     * @return 成功すれば true
     */
    static bool Save(IScene* scene, const std::string& sceneName);

    /**
     * @brief ファイルからシーンの状態を読み込む
     * @param scene 展開先のシーン
     * @param sceneName シーン名
     * @return 成功すれば true
     */
    static bool Load(IScene* scene, const std::string& sceneName);

    /**
     * @brief 指定したシーン名の JSON ファイルが存在するか確認する
     */
    static bool Exists(const std::string& sceneName);

private:
    /**
     * @brief シーン名からファイルパスを生成する (resources/scenes/[Name].json)
     */
    static std::string GetSceneFilePath(const std::string& sceneName);
};
