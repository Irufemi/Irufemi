#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class IrufemiEngine;
class IScene;

/**
 * @class SceneManager
 * @brief ゲームのシーン遷移とライフサイクルを管理するクラス
 * @details IScene を継承したシーンクラスの登録、切替、更新、描画を一括して行います。
 *          シーンのスタック管理ではなく、単一の現在シーンを保持する形式です。
 *          また、ゲーム全体の一時停止（ポーズ）フラグも保持します。
 */
class SceneManager {
public:
    /** @brief シーン名（識別子）の型定義 */
    using Key = std::string;
    /** @brief シーン生成関数の型定義 */
    using Factory = std::function<std::unique_ptr<IScene>()>;

    /**
     * @brief コンストラクタ
     * @param[in] engine IrufemiEngine へのポインタ（非所有）
     */
    explicit SceneManager(IrufemiEngine* engine);

    /** @name シーン登録・遷移 */
    ///@{
    /**
     * @brief シーンをファクトリ関数と共に登録する
     * @param[in] name シーンの識別名
     * @param[in] f シーンインスタンスを作成するラムダ式等の関数
     */
    void Register(const Key& name, Factory f);

    /**
     * @brief シーンの切替を要求する（次の Update 冒頭で反映）
     * @param[in] next 切り替え先のシーン名
     */
    void Request(const Key& next);

    /**
     * @brief 即時シーンを切り替える（初期化時などに使用）
     * @param[in] next 切り替え先のシーン名
     * @return 切り替えに成功したら true
     */
    bool ChangeTo(const Key& next);
    ///@}

    /** @name 更新・描画 */
    ///@{
    /**
     * @brief 現在のシーンの更新処理
     * @details シーン切替要求がある場合は、更新の前にシーンの差し替えを行います。
     */
    void Update();

    /**
     * @brief 現在のシーンの描画処理
     */
    void Draw();
    ///@}

    /** @name 状態取得 */
    ///@{
    /** @brief 現在のシーン名を取得 */
    const Key& GetCurrent() const;
    /** @brief 現在のシーンインスタンスを取得 */
    IScene* GetCurrentScene() const { return current_.get(); }


    /** @brief 登録済みの全シーン名を取得（登録順） */
    std::vector<Key> GetRegisteredKeys() const;
    ///@}

    /** @name ポーズ制御 */
    ///@{
    /** @brief ポーズ状態を反転させる */
    void TogglePause() { isPaused_ = !isPaused_; }
    /** @brief ポーズ中かどうかを取得 */
    bool IsPaused() const { return isPaused_; }

    /** @brief シーンの初期化（Initialize）実行中かどうかを取得 */
    bool IsInitializing() const { return isInitializing_; }
    ///@}

private:
    IrufemiEngine* engine_ = nullptr; ///< エンジン本体への参照

    std::unordered_map<Key, Factory> factories_; ///< シーン識別名と生成関数のマップ
    std::vector<Key> order_; ///< 登録されたシーン名のリスト（順序保持用）

    std::unique_ptr<IScene> current_{}; ///< 現在実行中のシーンインスタンス
    Key currentName_{};  ///< 現在のシーン名
    Key pending_{};      ///< 次フレームで切り替え予定のシーン名

    bool isPaused_ = false; ///< ゲーム全体の一時停止フラグ
    bool isInitializing_ = false; ///< シーンの初期化（Initialize）実行中フラグ
};