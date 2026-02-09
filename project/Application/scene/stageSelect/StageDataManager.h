#pragma once

/**
 * @class StageDataManager
 * @brief ステージ選択情報を管理する静的クラス
 * @details シーン間で選択されたステージ番号を共有するために使用されます。
 */
class StageDataManager {
public:
    /**
     * @brief 選択されたステージ番号 (0-indexed)
     * @details SelectSceneで設定され、GameSceneで読み込まれます。
     */
    static int selectedStageIndex;
};