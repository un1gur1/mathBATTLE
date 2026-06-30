#pragma once

#include <cstdlib>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "../Common/Vector2.h"
#include "../Object/Map/MapGrid.h"
#include "../Object/Unit/Enemy/Enemy.h"
#include "../Object/Unit/Player/Player.h"
#include "../Scene/PauseMenu.h"

namespace App {

    class BattleUI;
    class UnitBase;

    // ==========================================
    // Fraction: 分数計算用の構造体
    // - 小数誤差を出さずに四則演算結果を保持する
    // - コンストラクタで自動的に約分・正規化する
    // ==========================================
    struct Fraction {
        long long n;  // 分子
        long long d;  // 分母

        Fraction(long long num = 0, long long den = 1) : n(num), d(den) {
            Normalize();
        }

        Fraction operator+(const Fraction& other) const { return Fraction(n * other.d + other.n * d, d * other.d); }
        Fraction operator-(const Fraction& other) const { return Fraction(n * other.d - other.n * d, d * other.d); }
        Fraction operator*(const Fraction& other) const { return Fraction(n * other.n, d * other.d); }
        Fraction operator/(const Fraction& other) const { return Fraction(n * other.d, d * other.n); }

        bool operator==(const Fraction& other) const { return n == other.n && d == other.d; }
        bool operator>(const Fraction& other) const { return n * other.d > other.n * d; }
        bool operator<(const Fraction& other) const { return n * other.d < other.n * d; }

        std::string ToString() const {
            if (d == 1) return std::to_string(n);

            long long whole = n / d;
            long long rem = std::llabs(n % d);

            if (whole == 0) {
                return (n < 0 ? "-" : "") + std::to_string(rem) + "/" + std::to_string(d);
            }

            return std::to_string(whole) + (n < 0 ? " - " : " + ") + std::to_string(rem) + "/" + std::to_string(d);
        }

    private:
        static long long GcdAbs(long long a, long long b) {
            a = std::llabs(a);
            b = std::llabs(b);
            while (b != 0) {
                long long t = b;
                b = a % b;
                a = t;
            }
            return a;
        }

        void Normalize() {
            if (d == 0) {
                n = 0;
                d = 1;
                return;
            }

            if (d < 0) {
                n = -n;
                d = -d;
            }

            long long gcd = GcdAbs(n, d);
            if (gcd != 0) {
                n /= gcd;
                d /= gcd;
            }
        }
    };

    // ==========================================
    // BattleMaster: バトル全体を管理するクラス
    // - ターン制バトルのフロー制御
    // - 入力 / AI / 勝敗判定 / 演出の入口
    // ==========================================
    class BattleMaster {
        friend class BattleUI;

    public:
        enum class Phase {
            P1_Move,
            P1_Action,
            P2_Move,
            P2_Action,
            Result,
            FINISH
        };

        enum class GameMode {
            VS_CPU,
            VS_PLAYER
        };

        enum class RuleMode {
            CLASSIC,   // 残機制
            ZERO_ONE   // カウント制
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

    private:
        // ---------- ターン / ルール状態 ----------
        Phase    m_currentPhase;
        GameMode m_gameMode;
        RuleMode m_ruleMode;
        int      m_maxTurns = 0;
        bool     m_isPaused = false;

        // ---------- フィールド / ユニット / UI ----------
        MapGrid m_mapGrid;
        std::unique_ptr<BattleUI> m_ui;
        std::unique_ptr<Player>   m_player;
        std::unique_ptr<Enemy>    m_enemy;
        PauseMenu m_pauseMenu;

        // ---------- カウント制用スコア ----------
        Fraction m_p1ZeroOneScore;
        Fraction m_p2ZeroOneScore;
        int      m_targetScore;

        // ---------- 入力 / 選択状態 ----------
        bool       m_isPlayerSelected;
        IntVector2 m_hoverGrid;
        int        m_logScrollOffset = 0;

        // ---------- CPU / AI 状態 ----------
        bool m_enemyAIStarted;
        bool m_playerAIStarted;
        bool m_is1P_NPC;
        bool m_is2P_NPC;
        int  g_aiStayCount1P;
        int  g_aiStayCount2P;

        // ---------- 演算子維持コスト ----------
        // ターン開始時に演算子を保持していた場合に true。
        // ターン終了時、まだ演算子を保持していればバッテリーを1消費する。
        bool m_p1OpCostPending = false;
        bool m_p2OpCostPending = false;

        bool m_isBattleFinished = false;
        bool m_is1PWinner = false;

        // ---------- 演出 ----------
        int   m_finishTimer;
        int   m_psHandle;
        int   m_cbHandle;
        float m_shaderTime;
        float m_effectIntensity;
        float m_uiCursorX_1P = 0.0f;
        float m_uiCursorX_2P = 0.0f;

        // ---------- 戦績 ----------
        int m_totalMoves;
        int m_totalOps;
        int m_maxDamage;
        int m_startTime;
        std::vector<std::string> m_actionLog;

        // ---------- ターン補助 ----------
        bool Is1PTurn() const;
        UnitBase* GetActiveUnit() const;
        UnitBase* GetTargetUnit() const;
        UnitBase* GetUnitBySide(bool is1P) const;

        void ReserveOperatorUpkeepIfNeeded(UnitBase& unit, bool is1P);
        void ApplyOperatorUpkeepCost(bool is1P);
        void FinishActionPhase(bool is1P, Phase nextTurnPhase);
        void AddPowerWithBattery(UnitBase& unit, int delta, const std::string& reason);
        void SetClassicDefeat(UnitBase& loser, const std::string& reason);

        // ---------- 入力処理 ----------
        void HandleMoveInput(UnitBase& activeUnit, Phase nextPhase);
        void HandleActionInput(UnitBase& actor, UnitBase& targetUnit);
        bool CheckButtonClick(int x, int y, int w, int h, const Vector2& mousePos) const;

        // ---------- バトル処理 ----------
        bool CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const;
        void ExecuteBattle(UnitBase& attacker, UnitBase& defender, UnitBase& target);
        void ApplyBattleResult(UnitBase& unit, const Fraction& resultFrac, int intRes, char op);
        void AddLog(const std::string& message);

        // ---------- AI ----------
        int  EvaluateBoard(const UnitBase& me, int myVirtualNumber, const UnitBase& enemy, IntVector2 targetPos, bool is1P) const;
        void ExecuteAI(UnitBase* me, UnitBase* opp, bool is1P);
        void PerformAIMove(UnitBase* me, IntVector2 bestTarget, int selectedCost, bool is1P);
        void ExecuteAIAction(UnitBase* me, UnitBase* opp, bool is1P);

        // ---------- 描画補助 ----------
        void DrawMovableArea() const;
        void DrawEnemyDangerArea() const;
    };

} // namespace App
