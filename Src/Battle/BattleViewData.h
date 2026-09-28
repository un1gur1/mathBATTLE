#pragma once

#include <array>
#include <string>
#include <vector>

#include "../Common/Vector2.h"

namespace App {

    class MapGrid;
    class UnitBase;

    // BattleMaster が計算した「表示専用データ」。
    // BattleUI はこの構造体だけを受け取り、ゲームルールを再計算しない。
    enum class BattleViewPhase {
        P1_TurnStart,
        P1_Move,
        P1_Action,
        P2_TurnStart,
        P2_Move,
        P2_Action,
        FINISH
    };

    enum class BattleViewRuleMode {
        CLASSIC,
        ZERO_ONE,
        ROUND_BATTLE
    };

    enum class BattleRoundViewPhase {
        INACTIVE,
        ROUND_START,
        SELECT_P1_NUMBER,
        SELECT_P2_NUMBER,
        TARGET_REVEAL,
        DRAFT_P1_OPERATOR,
        DRAFT_P2_OPERATOR,
        PLACE_OPERATORS,
        BATTLE,
        ROUND_END
    };

    struct FractionView {
        long long n = 0;
        long long d = 1;
    };

    struct BattleMoveCellView {
        IntVector2 pos{ -1, -1 };
        int cost = 0;
        bool baseReachable = false;
        bool warp = false;
    };

    struct BattleTargetResultView {
        bool valid = false;
        bool defeated = false;

        int number = 0;
        int stocks = 0;
        int stockDelta = 0;

        bool hasScore = false;
        FractionView score{};
    };

    struct BattleCalculationView {
        bool visible = false;
        bool movePreview = false;
        bool willGetNewOperator = false;

        int leftNumber = 0;
        int rightNumber = 0;
        char op = '\0';

        int intResult = 0;
        FractionView fractionResult{};
        bool cleanDivide = true;

        bool createsWarp = false;
        // 内部グリッド座標
        IntVector2 warpGrid{ -1, -1 };
        // プレイヤーに見せる 1-9 の論理座標
        IntVector2 warpDisplay{ -1, -1 };

        bool hoverSelf = false;
        bool hoverEnemy = false;

        BattleTargetResultView selfResult{};
        BattleTargetResultView enemyResult{};
    };

    struct BattleUnitView {
        // DrawUnitGraphic を呼ぶためだけに保持する。
        // ルール判断には使用しない。
        UnitBase* unit = nullptr;

        bool isNPC = false;
        bool activeTurn = false;

        int number = 0;
        int stocks = 0;
        int maxStocks = 0;
        char op = '\0';
        IntVector2 gridPos{ -1, -1 };

        FractionView score{};
        int displayScore = 0;

        int moveDistance = 0;
        std::array<bool, 9> moveDirectionDots{};

        bool hasPowerPreview = false;
        int previewNumber = 0;
        int previewStocks = 0;
        bool previewDefeated = false;
        int previewMoveDistance = 0;
        std::array<bool, 9> previewMoveDirectionDots{};

        bool hasScorePreview = false;
        FractionView previewScore{};

        std::vector<IntVector2> warpNodes;
    };

    struct BattleViewData {
        const MapGrid* map = nullptr;

        BattleViewPhase phase = BattleViewPhase::P1_TurnStart;
        BattleViewRuleMode ruleMode = BattleViewRuleMode::CLASSIC;

        bool is1PTurn = true;
        bool gameOver = false;
        bool playerSelected = false;

        // 新ラウンドバトル用の表示状態。
        bool roundSetupActive = false;
        BattleRoundViewPhase roundPhase = BattleRoundViewPhase::INACTIVE;
        int roundNumber = 0;
        int p1RoundStartNumber = 0;
        int p2RoundStartNumber = 0;
        int roundTarget = 0;
        int roundNumberCursor = 5;
        bool roundWaitingForRemote = false;

        // 演算子ドラフト / 自動配置
        std::array<char, 4> roundOperators{ '+', '-', '*', '/' };
        std::array<bool, 4> roundOperatorAvailable{ true, true, true, true };
        int roundOperatorCursor = 0;
        char p1DraftedOperator = '\0';
        char p2DraftedOperator = '\0';
        char roundPlacedOperator1 = '\0';
        char roundPlacedOperator2 = '\0';
        IntVector2 roundPlacedPos1{ -1, -1 };
        IntVector2 roundPlacedPos2{ -1, -1 };
        int roundWinner = 0;
        int p1RoundWins = 0;
        int p2RoundWins = 0;

        char p1FixedOperator = '\0';
        char p2FixedOperator = '\0';
        char p1SubOperator = '\0';
        char p2SubOperator = '\0';
        char p1TurnOperator = '\0';
        char p2TurnOperator = '\0';
        bool p1UsingSub = false;
        bool p2UsingSub = false;

        bool roundResultPending = false;
        bool roundChipPlacementPending = false;
        bool roundPendingIs1P = true;
        int roundPendingResult = 0;
        int roundPendingRaw = 0;
        bool roundCanAddTotal = false;
        bool hoverRoundFixedButton = false;
        bool hoverRoundSubButton = false;
        bool hoverRoundPlayerCalcButton = false;
        bool hoverRoundChipCalcButton = false;
        bool hoverRoundTotalButton = false;
        bool hoverRoundChipButton = false;
        bool roundPlayerCalcAvailable = false;
        bool roundChipCalcAvailable = false;
        int roundChipOperand = 0;
        std::vector<IntVector2> roundChipPlacementCells;


        int totalTurns = 0;
        int targetScore = 0;
        int turnStartTimer = 0;
        int finishTimer = 0;

        IntVector2 hoverGrid{ -1, -1 };

        BattleUnitView p1{};
        BattleUnitView p2{};

        bool hasActiveUnit = false;
        bool activeUnitIs1P = true;
        IntVector2 activeGrid{ -1, -1 };

        std::vector<BattleMoveCellView> enemyDangerCells;
        std::vector<BattleMoveCellView> movableCells;

        bool hoverMoveValid = false;
        IntVector2 hoverMoveGrid{ -1, -1 };
        int hoverMoveCost = 0;

        bool canAttack = false;
        bool hasOperator = false;
        bool humanActionPhase = false;

        bool hoverNoActionButton = false;
        bool hoverEndTurnButton = false;
        bool hoverPauseButton = false;

        BattleCalculationView calculation{};

        // UIにルール文言を描かせるだけにするため、文言もMaster側で渡す。
        std::array<std::string, 5> ruleLines{};
    };

} // namespace App
