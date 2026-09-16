#include "BattleSetup.h"

namespace App {

    namespace {
        constexpr int TARGET_SCORES[3] = { 53, 103, 223 };

        constexpr int DEF_P1_HP = 5;
        constexpr int DEF_P1_X = 3;
        constexpr int DEF_P1_Y = 3;

        constexpr int DEF_P2_HP = 5;
        constexpr int DEF_P2_X = 7;
        constexpr int DEF_P2_Y = 7;
    }

    void BattleSetup::Init() {
        step = Step::SELECT_PLAYERS;
        playerCursor = 1;
        modeCursor = 0;
        stocksCursor = 0;
        scoreCursor = 1;
        stageCursor = 0;

        players[0] = { 0, 0, DEF_P1_HP, DEF_P1_X, DEF_P1_Y };
        players[1] = { 1, 0, DEF_P2_HP, DEF_P2_X, DEF_P2_Y };
    }

    int BattleSetup::GetSelectedMaxStocks() const {
        return (stocksCursor == 0) ? 1 : (stocksCursor == 1) ? 3 : 5;
    }

    int BattleSetup::GetSelectedTargetScore() const {
        int index = scoreCursor;
        if (index < 0) index = 0;
        if (index > 2) index = 2;
        return TARGET_SCORES[index];
    }

} // namespace App
