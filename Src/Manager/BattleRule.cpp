#include "BattleRule.h"

#include <algorithm>
#include <cmath>

namespace App {

    BattleRule::BattleRule() = default;

    void BattleRule::Configure(BattleRuleKind kind, int targetScore) {
        m_kind = kind;
        m_targetScore = targetScore;
    }

    int BattleRule::CalculateRoundTarget(int p1StartNumber, int p2StartNumber) const {
        p1StartNumber = std::clamp(p1StartNumber, 1, 9);
        p2StartNumber = std::clamp(p2StartNumber, 1, 9);
        return 9 + p1StartNumber + p2StartNumber;
    }

    bool BattleRule::IsRoundTargetReached(const BattleFraction& score, int roundTarget) const {
        return score == BattleFraction(roundTarget);
    }

    int BattleRule::GetMoveDistance(int number) const {
        if (number < 1 || number > 9) return 0;
        return 3 - ((number - 1) % 3);
    }

    bool BattleRule::CanMove(
        int number,
        char op,
        IntVector2 start,
        IntVector2 target,
        bool isWarpNode,
        int& outCost) const {

        outCost = 0;
        if (start == target) return false;

        if (isWarpNode) {
            outCost = 1;
            return true;
        }

        const int dx = std::abs(target.x - start.x);
        const int dy = std::abs(target.y - start.y);
        const int maxDist = GetMoveDistance(number);
        if (maxDist <= 0) return false;

        bool validDirection = false;
        bool isJump = false;

        if (number >= 1 && number <= 3) {
            validDirection = (dx == 0 || dy == 0);
        }
        else if (number >= 4 && number <= 6) {
            validDirection = (dx == dy);
        }
        else if (number >= 7 && number <= 9) {
            validDirection = (dx == 0 || dy == 0 || dx == dy);
        }

        if (op == '+') {
            validDirection |= (dx == 0 || dy == 0);
        }
        else if (op == '-') {
            validDirection |= (dy == 0);
        }
        else if (op == '*') {
            validDirection |= (dx == dy);
        }
        else if (op == '/') {
            validDirection |= (dy == 0);
            if (dx == 0 && dy == 2) {
                validDirection = true;
                isJump = true;
            }
        }

        if (!validDirection) return false;

        if (isJump) {
            outCost = 2;
            return true;
        }

        if (dx <= maxDist && dy <= maxDist) {
            outCost = (std::max)(dx, dy);
            return true;
        }

        return false;
    }

    bool BattleRule::IsAdjacent(IntVector2 a, IntVector2 b) const {
        return (std::abs(a.x - b.x) + std::abs(a.y - b.y) == 1);
    }

    std::array<bool, 9> BattleRule::BuildBaseMoveDirectionDots(int number) const {
        std::array<bool, 9> dots{};
        for (int i = 0; i < 9; ++i) {
            if (i == 4) {
                dots[i] = false;
                continue;
            }

            const int gx = i % 3;
            const int gy = i / 3;

            if (number >= 1 && number <= 3) {
                dots[i] = (gx == 1 || gy == 1);
            }
            else if (number >= 4 && number <= 6) {
                dots[i] = (gx == gy || gx + gy == 2);
            }
            else if (number >= 7 && number <= 9) {
                dots[i] = true;
            }
            else {
                dots[i] = false;
            }
        }
        return dots;
    }

    BattleCalculationResult BattleRule::CalculateBattleResult(
        int attackerNumber,
        int defenderNumber,
        char op) const {

        BattleCalculationResult result;

        if (op == '+') {
            result.intValue = attackerNumber + defenderNumber;
            result.fraction = BattleFraction(result.intValue);
        }
        else if (op == '-') {
            result.intValue = attackerNumber - defenderNumber;
            result.fraction = BattleFraction(result.intValue);
        }
        else if (op == '*') {
            result.intValue = attackerNumber * defenderNumber;
            result.fraction = BattleFraction(result.intValue);
        }
        else if (op == '/') {
            if (defenderNumber != 0 && attackerNumber % defenderNumber == 0) {
                result.intValue = attackerNumber / defenderNumber;
                result.fraction = BattleFraction(result.intValue);
            }
            else {
                result.intValue = 0;
                result.fraction = BattleFraction(0);
                result.cleanDivide = false;
            }
        }

        return result;
    }


    BattleCalculationResult BattleRule::CalculateRoundArithmeticResult(
        int leftNumber,
        int rightNumber,
        char op) const {

        BattleCalculationResult result;
        result.valid = false;
        result.cleanDivide = true;

        int raw = 0;
        if (op == '+') raw = leftNumber + rightNumber;
        else if (op == '-') raw = std::abs(leftNumber - rightNumber);
        else if (op == '*') raw = leftNumber * rightNumber;
        else if (op == '/') {
            if (rightNumber == 0 || leftNumber % rightNumber != 0) {
                result.cleanDivide = false;
                return result;
            }
            raw = leftNumber / rightNumber;
        }
        else {
            return result;
        }

        // 0は初版ROUND_BATTLEでは扱わない。
        if (raw <= 0) return result;

        result.valid = true;
        result.rawValue = raw;
        result.normalizedValue = NormalizeRoundValue(raw);
        result.intValue = raw;
        result.fraction = BattleFraction(raw);
        return result;
    }

    int BattleRule::NormalizeRoundValue(int value) const {
        if (value <= 0) return 0;
        return 1 + ((value - 1) % 9);
    }

    IntVector2 BattleRule::GetWarpGrid(int attackerNumber, int defenderNumber) const {
        return IntVector2{ attackerNumber - 1, 9 - defenderNumber };
    }

    BattleFraction BattleRule::CalculateBouncedScore(
        const BattleFraction& currentScore,
        const BattleFraction& addition) const {
        return CalculateBouncedScoreToTarget(currentScore, addition, m_targetScore);
    }

    BattleFraction BattleRule::CalculateBouncedScoreToTarget(
        const BattleFraction& currentScore,
        const BattleFraction& addition,
        int targetScore) const {

        const BattleFraction goal(targetScore);
        BattleFraction nextScore = currentScore + addition;
        if (nextScore > goal) {
            nextScore = goal - (nextScore - goal);
        }
        return nextScore;
    }

    int BattleRule::WrapPower1To9(int value) const {
        int cycleValue = (value - 1) % 9;
        if (cycleValue < 0) cycleValue += 9;
        return cycleValue + 1;
    }

    BattleSimulatedUnitState BattleRule::SimulateClassicResult(
        int currentNumber,
        int currentStocks,
        int maxStocks,
        int intResult,
        char op,
        bool isCleanDivide) const {

        BattleSimulatedUnitState out;
        out.number = currentNumber;
        out.stocks = currentStocks;

        if (op == '/' && !isCleanDivide) return out;

        out.valid = true;
        int newPower = intResult;
        int stockChange = 0;

        while (newPower <= 0) {
            --stockChange;
            newPower += 9;
        }
        while (newPower > 9) {
            ++stockChange;
            newPower -= 9;
        }

        if (currentStocks + stockChange < 0) {
            out.defeated = true;
            out.number = 0;
            out.stocks = currentStocks + stockChange;
            out.stockDelta = stockChange;
            return out;
        }

        out.number = newPower;
        out.stocks = std::clamp(currentStocks + stockChange, 0, maxStocks);
        out.stockDelta = out.stocks - currentStocks;
        return out;
    }

    BattleSimulatedUnitState BattleRule::SimulateZeroOneResult(
        int currentNumber,
        int currentStocks,
        const BattleFraction& currentScore,
        int intResult,
        const BattleFraction& resultFrac,
        char op,
        bool isCleanDivide) const {

        BattleSimulatedUnitState out;
        out.number = currentNumber;
        out.stocks = currentStocks;
        out.score = currentScore;

        if (op == '/' && !isCleanDivide) return out;

        out.valid = true;
        out.hasScore = true;
        out.score = CalculateBouncedScore(currentScore, resultFrac);
        out.number = WrapPower1To9(intResult);
        return out;
    }

    BattleSimulatedUnitState BattleRule::SimulateRoundMoveCost(
        int currentNumber,
        int currentStocks,
        int cost) const {

        BattleSimulatedUnitState out;
        out.valid = true;
        out.number = WrapPower1To9(currentNumber - (std::max)(1, cost));
        out.stocks = currentStocks;
        out.stockDelta = 0;
        return out;
    }

    BattleSimulatedUnitState BattleRule::SimulateRoundBattleResult(
        int currentNumber,
        int currentStocks,
        const BattleFraction& currentScore,
        int roundTarget,
        int intResult,
        const BattleFraction& resultFrac,
        char op,
        bool isCleanDivide) const {

        (void)resultFrac;

        BattleSimulatedUnitState out;
        out.number = currentNumber;
        out.stocks = currentStocks;
        out.score = currentScore;
        out.hasScore = true;

        if (op == '/' && !isCleanDivide) return out;

        const int normalized = NormalizeRoundValue(intResult);
        if (normalized <= 0) return out;

        const BattleFraction nextScore = currentScore + BattleFraction(normalized);
        if (nextScore > BattleFraction(roundTarget)) return out;

        out.valid = true;
        out.number = normalized;
        out.score = nextScore;
        return out;
    }

    BattleSimulatedUnitState BattleRule::SimulateMoveCost(
        int currentNumber,
        int currentStocks,
        int maxStocks,
        int cost) const {

        BattleSimulatedUnitState out;
        out.valid = true;
        out.number = currentNumber - cost;
        out.stocks = currentStocks;

        if (out.number <= 0) {
            if (currentStocks <= 0) {
                out.defeated = true;
                out.number = 0;
                out.stocks = -1;
                out.stockDelta = -1;
                return out;
            }

            out.stocks = std::clamp(currentStocks - 1, 0, maxStocks);
            out.stockDelta = out.stocks - currentStocks;
            while (out.number <= 0) out.number += 9;
        }
        else if (out.number > 9) {
            out.stocks = std::clamp(currentStocks + 1, 0, maxStocks);
            out.stockDelta = out.stocks - currentStocks;
            while (out.number > 9) out.number -= 9;
        }

        return out;
    }

    bool BattleRule::IsGameOver(
        const BattleFraction& p1Score,
        const BattleFraction& p2Score,
        bool battleFinished) const {

        if (IsZeroOne()) {
            const BattleFraction goal(m_targetScore);
            return p1Score == goal || p2Score == goal;
        }
        if (IsRoundBattle()) return battleFinished;
        return battleFinished;
    }

    bool BattleRule::IsP1Winner(
        const BattleFraction& p1Score,
        const BattleFraction& p2Score,
        bool p1Winner) const {

        (void)p2Score;
        if (IsZeroOne()) {
            const BattleFraction goal(m_targetScore);
            return p1Score == goal;
        }
        if (IsRoundBattle()) return p1Winner;
        return p1Winner;
    }

    std::array<std::string, 5> BattleRule::GetRuleLines() const {
        if (IsRoundBattle()) {
            return {
                "目標値 = 9 + P1初期数字 + P2初期数字",
                "固定演算子は何度でも使用 / サブ演算子は使用したターンで消費",
                "計算結果を1〜9へ正規化し、合計値へ加算するか数字チップ化",
                "通常移動は最初のアイテムマスまで。÷ジャンプとワープは通過可",
                "3ラウンド制2本先取：先に2ラウンド取ったプレイヤーが勝利"
            };
        }
        return {
            "距離 [1,4,7]=3 / [2,5,8]=2 / [3,6,9]=1",
            "方向 [1-3]=十字 / [4-6]=斜め / [7-9]=全方向",
            "演算子 +:十字 / -:横 / *:斜め / /:横+縦2ジャンプ",
            "演算子取得で攻撃・移動ルート拡張",
            "/ は (自分,相手) の座標にワープを設置"
        };
    }

} // namespace App
