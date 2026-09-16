#pragma once

#include <array>
#include <cstdlib>
#include <string>

#include "../Common/Vector2.h"

namespace App {

    enum class BattleRuleKind {
        CLASSIC,
        ZERO_ONE,
        ROUND_BATTLE
    };

    struct BattleFraction {
        long long n = 0;
        long long d = 1;

        BattleFraction(long long num = 0, long long den = 1)
            : n(num), d(den) {
            Normalize();
        }

        BattleFraction operator+(const BattleFraction& other) const {
            return BattleFraction(n * other.d + other.n * d, d * other.d);
        }
        BattleFraction operator-(const BattleFraction& other) const {
            return BattleFraction(n * other.d - other.n * d, d * other.d);
        }
        BattleFraction operator*(const BattleFraction& other) const {
            return BattleFraction(n * other.n, d * other.d);
        }
        BattleFraction operator/(const BattleFraction& other) const {
            return BattleFraction(n * other.d, d * other.n);
        }

        bool operator==(const BattleFraction& other) const {
            return n == other.n && d == other.d;
        }
        bool operator!=(const BattleFraction& other) const {
            return !(*this == other);
        }
        bool operator>(const BattleFraction& other) const {
            return n * other.d > other.n * d;
        }
        bool operator<(const BattleFraction& other) const {
            return n * other.d < other.n * d;
        }

        std::string ToString() const {
            if (d == 1) return std::to_string(n);

            const long long whole = n / d;
            const long long rem = std::llabs(n % d);
            if (whole == 0) {
                return (n < 0 ? "-" : "") + std::to_string(rem) + "/" + std::to_string(d);
            }
            return std::to_string(whole) +
                (n < 0 ? " - " : " + ") +
                std::to_string(rem) + "/" + std::to_string(d);
        }

    private:
        static long long GcdAbs(long long a, long long b) {
            a = std::llabs(a);
            b = std::llabs(b);
            while (b != 0) {
                const long long t = b;
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
            const long long gcd = GcdAbs(n, d);
            if (gcd != 0) {
                n /= gcd;
                d /= gcd;
            }
        }
    };

    using Fraction = BattleFraction;

    struct BattleCalculationResult {
        BattleFraction fraction{ 0, 1 };
        int intValue = 0;
        bool cleanDivide = true;
    };

    struct BattleSimulatedUnitState {
        bool valid = false;
        bool defeated = false;
        int number = 0;
        int stocks = 0;
        int stockDelta = 0;
        bool hasScore = false;
        BattleFraction score{ 0, 1 };
    };

    class BattleRule {
    public:
        BattleRule();

        void Configure(BattleRuleKind kind, int targetScore);

        BattleRuleKind GetKind() const { return m_kind; }
        bool IsClassic() const { return m_kind == BattleRuleKind::CLASSIC; }
        bool IsZeroOne() const { return m_kind == BattleRuleKind::ZERO_ONE; }
        bool IsRoundBattle() const { return m_kind == BattleRuleKind::ROUND_BATTLE; }
        int GetTargetScore() const { return m_targetScore; }

        int CalculateRoundTarget(int p1StartNumber, int p2StartNumber) const;
        bool IsRoundTargetReached(const BattleFraction& score, int roundTarget) const;

        bool UsesOperatorUpkeep() const { return IsClassic(); }

        int GetMoveDistance(int number) const;
        bool CanMove(
            int number,
            char op,
            IntVector2 start,
            IntVector2 target,
            bool isWarpNode,
            int& outCost) const;
        bool IsAdjacent(IntVector2 a, IntVector2 b) const;
        std::array<bool, 9> BuildBaseMoveDirectionDots(int number) const;

        BattleCalculationResult CalculateBattleResult(
            int attackerNumber,
            int defenderNumber,
            char op) const;
        IntVector2 GetWarpGrid(int attackerNumber, int defenderNumber) const;

        BattleFraction CalculateBouncedScore(
            const BattleFraction& currentScore,
            const BattleFraction& addition) const;
        BattleFraction CalculateBouncedScoreToTarget(
            const BattleFraction& currentScore,
            const BattleFraction& addition,
            int targetScore) const;
        int WrapPower1To9(int value) const;

        BattleSimulatedUnitState SimulateClassicResult(
            int currentNumber,
            int currentStocks,
            int maxStocks,
            int intResult,
            char op,
            bool isCleanDivide) const;

        BattleSimulatedUnitState SimulateZeroOneResult(
            int currentNumber,
            int currentStocks,
            const BattleFraction& currentScore,
            int intResult,
            const BattleFraction& resultFrac,
            char op,
            bool isCleanDivide) const;

        // ラウンドバトル：STOCKはラウンド残機なので、移動/演算では増減させない。
        BattleSimulatedUnitState SimulateRoundMoveCost(
            int currentNumber,
            int currentStocks,
            int cost) const;

        // ラウンドバトル：演算結果を対象のラウンドスコアへ加算し、
        // パワーは1-9へ循環させる。割り切れない÷は無効。
        BattleSimulatedUnitState SimulateRoundBattleResult(
            int currentNumber,
            int currentStocks,
            const BattleFraction& currentScore,
            int roundTarget,
            int intResult,
            const BattleFraction& resultFrac,
            char op,
            bool isCleanDivide) const;

        BattleSimulatedUnitState SimulateMoveCost(
            int currentNumber,
            int currentStocks,
            int maxStocks,
            int cost) const;

        bool IsGameOver(
            const BattleFraction& p1Score,
            const BattleFraction& p2Score,
            bool battleFinished) const;
        bool IsP1Winner(
            const BattleFraction& p1Score,
            const BattleFraction& p2Score,
            bool p1Winner) const;

        std::array<std::string, 5> GetRuleLines() const;

    private:
        BattleRuleKind m_kind = BattleRuleKind::CLASSIC;
        int m_targetScore = 53;
    };

} // namespace App
