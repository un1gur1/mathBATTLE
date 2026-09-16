#pragma once

namespace App {

    // タイトル画面で編集する「試合開始前設定」だけを保持するモデル。
    // 旧ノーマル/カウント設定は新ルール完成までは互換性のため残す。
    class BattleSetup {
    public:
        static constexpr int GRID_SIZE = 9;
        static constexpr int ROUND_BATTLE_MODE = 2;
        static constexpr int ROUND_DEFAULT_STOCKS = 3;

        enum class Step {
            SELECT_PLAYERS,
            SELECT_MODE,
            SELECT_CLASSIC_STOCKS,
            SELECT_SCORE,
            SELECT_P1_TYPE,
            SELECT_P2_TYPE,
            SELECT_STAGE,
            CUSTOM_P1_START,
            CUSTOM_P2_START
        };

        struct PlayerConfig {
            int typeCursor = 0;
            int customCursor = 0;
            int startNum = 5;
            int startX = 1;
            int startY = 1;
        };

        void Init();

        int GetSelectedMaxStocks() const;
        int GetSelectedTargetScore() const;
        bool IsRoundBattle() const { return modeCursor == ROUND_BATTLE_MODE; }
        int GetRoundBattleStocks() const { return ROUND_DEFAULT_STOCKS; }

        Step step = Step::SELECT_PLAYERS;
        int playerCursor = 1;
        int modeCursor = 0;
        int stocksCursor = 0;
        int scoreCursor = 1;
        int stageCursor = 0;
        PlayerConfig players[2];
    };

} // namespace App
