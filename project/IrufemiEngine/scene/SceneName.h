#pragma once

/// <summary>
/// シーン名
/// </summary>
enum class SceneName {
    title = 0,
    inGame,
    result,

    CountOfSceneName,
};

//タイトル→キャラセレクト→インゲーム→リザルト→選択(もう一度プレイ、キャラセレクト、タイトルに戻る)