#pragma once

#include <array>
#include <memory>
#include <queue>
#include <string>

#include "BattleRule.h"
#include "../Battle/BattleViewData.h"
#include "../Common/Vector2.h"
#include "../Object/Map/MapGrid.h"
#include "../Object/Unit/Enemy/Enemy.h"
#include "../Object/Unit/Player/Player.h"

namespace App {

    class BattleUI;
    class BattleAI;
    class UnitBase;
    enum class NetAction;

    // ==========================================
    // BattleMaster
    // バトル全体の進行と副作用を担当する。
    // ルール判定・数値計算は BattleRule に委譲する。
    // UI描画ロジックは持たず、BattleViewData を生成して UI に渡す。
    // ==========================================
    class BattleMaster {
    public:
        enum class Phase {
            P1_TurnStart,
            P1_Move,
            P1_Action,
            P2_TurnStart,
            P2_Move,
            P2_Action,
            FINISH
        };

        enum class GameMode {
            VS_CPU,
            VS_PLAYER
        };

        enum class RoundPhase {
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

        BattleMaster();
        ~BattleMaster();

        void Init();
        void Update();
        void Draw() const;

        bool IsGameOver() const;
        bool IsPlayerWin() const;

        const MapGrid& GetMapGrid() const { return m_mapGrid; }
        MapGrid& GetMapGrid() { return m_mapGrid; }

        int GetP1DisplayScore() const { return static_cast<int>(m_p1DisplayScore); }
        int GetP2DisplayScore() const { return static_cast<int>(m_p2DisplayScore); }
        int GetTurnStartTimer() const { return m_turnStartTimer; }

    private:
        // ---------- ターン / ルール状態 ----------
        Phase m_currentPhase;
        GameMode m_gameMode;
        BattleRule m_rule;

        // ---------- 新ラウンドバトル ----------
        // タイトルでは試合種別だけ決め、ラウンド固有の選択はBattle側で管理する。
        RoundPhase m_roundPhase = RoundPhase::INACTIVE;
        int m_roundNumber = 0;
        int m_p1RoundStartNumber = 0;
        int m_p2RoundStartNumber = 0;
        int m_roundTarget = 0;
        int m_roundNumberCursor = 5;
        int m_roundSetupWaitTimer = 0;

        Fraction m_p1RoundScore{ 0, 1 };
        Fraction m_p2RoundScore{ 0, 1 };

        std::array<char, 4> m_roundOperators{ '+', '-', '*', '/' };
        std::array<bool, 4> m_roundOperatorAvailable{ true, true, true, true };
        int m_roundOperatorCursor = 0;
        char m_p1DraftedOperator = '\0';
        char m_p2DraftedOperator = '\0';
        char m_roundPlacedOperator1 = '\0';
        char m_roundPlacedOperator2 = '\0';
        IntVector2 m_roundPlacedPos1{ -1, -1 };
        IntVector2 m_roundPlacedPos2{ -1, -1 };
        int m_roundWinner = 0;

        IntVector2 m_p1RoundStartPos{ -1, -1 };
        IntVector2 m_p2RoundStartPos{ -1, -1 };

        // ---------- フィールド / ユニット / UI ----------
        MapGrid m_mapGrid;
        std::unique_ptr<BattleUI> m_ui;
        std::unique_ptr<BattleAI> m_ai;
        std::unique_ptr<Player> m_player;
        std::unique_ptr<Enemy> m_enemy;

        // ---------- 旧カウント制用スコア ----------
        // 新ルール一本化後に不要なら削除する。
        Fraction m_p1ZeroOneScore;
        Fraction m_p2ZeroOneScore;

        // スコア表示の補間値。勝敗演出との同期に使用する。
        float m_p1DisplayScore;
        float m_p2DisplayScore;

        // ---------- 入力 / 選択状態 ----------
        bool m_isPlayerSelected;
        IntVector2 m_hoverGrid;

        // ---------- CPU / AI 状態 ----------
        bool m_enemyAIStarted;
        bool m_playerAIStarted;
        bool m_is1P_NPC;
        bool m_is2P_NPC;

        // ---------- 旧ノーマルの演算子維持コスト ----------
        bool m_p1OpCostPending = false;
        bool m_p2OpCostPending = false;

        // ---------- 勝敗 ----------
        bool m_isBattleFinished = false;
        bool m_is1PWinner = false;

        // ---------- 演出 ----------
        int m_finishTimer;
        float m_effectIntensity;

        // ---------- 戦績 ----------
        int m_p1TotalMoves, m_p2TotalMoves;
        int m_p1TotalOps, m_p2TotalOps;
        int m_p1MaxDamage, m_p2MaxDamage;
        int m_startTime;

        int m_turnStartTimer;
        int m_aiWaitTimer;

        // ---------- ターン補助 ----------
        bool Is1PTurn() const;
        UnitBase* GetActiveUnit() const;
        UnitBase* GetTargetUnit() const;
        UnitBase* GetUnitBySide(bool is1P) const;

        void ReserveOperatorUpkeepIfNeeded(UnitBase& unit, bool is1P);
        void ApplyOperatorUpkeepCost(bool is1P);
        void FinishActionPhase(bool is1P);
        void AddPowerWithBattery(UnitBase& unit, int delta, const std::string& reason);
        void SetClassicDefeat(UnitBase& loser, const std::string& reason);

        // ---------- 入力処理 ----------
        void HandleMoveInput(UnitBase& activeUnit, Phase nextPhase);
        void HandleActionInput(UnitBase& actor, UnitBase& targetUnit);
        bool CheckButtonClick(int x, int y, int w, int h, const Vector2& mousePos) const;

        // ---------- バトル処理 ----------
        // WarpNodeの所有確認だけMasterが行い、通常の合法手判定はRuleへ委譲する。
        bool CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const;
        void ExecuteBattle(UnitBase& attacker, UnitBase& defender, UnitBase& target);
        void ApplyBattleResult(UnitBase& unit, const Fraction& resultFrac, int intRes, char op, bool isCleanDivide);
        void AddLog(const std::string& message);

        // ---------- ラウンドセットアップ ----------
        void UpdateRoundSetup();
        int ReadRoundNumberKey() const;
        int ReadRoundOperatorKey() const;
        void ConfirmRoundStartNumber(bool is1P, int number);
        void ConfirmRoundOperator(bool is1P, char op);
        void PlaceRemainingRoundOperators();
        void BeginRoundBattle();
        void EndRound(int winnerSide);
        void PrepareNextRound();
        void ResetRoundBoardState();
        bool IsOnlineBattle() const;
        bool IsLocalRoundController(bool is1P) const;
        void SendRoundSelection(NetAction action, int value) const;
        bool ReceiveRoundSelection(NetAction action, int& value) const;
        int FindRoundOperatorIndex(char op) const;
        int FindNextAvailableRoundOperatorIndex(int from, int direction) const;

        // ---------- UI表示用 ----------
        BattleViewData BuildBattleViewData() const;

        // ---------- AI ----------
        void ExecuteAI(UnitBase* me, UnitBase* opp, bool is1P);
        void PerformAIMove(UnitBase* me, IntVector2 bestTarget, int selectedCost, bool is1P);
        void ExecuteAIAction(UnitBase* me, UnitBase* opp, bool is1P);
    };

} // namespace App
