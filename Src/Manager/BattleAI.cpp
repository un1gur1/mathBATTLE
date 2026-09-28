#define NOMINMAX
#include "BattleAI.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

#include "../Object/Map/MapGrid.h"
#include "../Object/Unit/UnitBase.h"

namespace App {

    namespace {
        constexpr std::array<char, 4> OPS{ '+', '-', '*', '/' };

        long long DistanceToGoal(const Fraction& score, const Fraction& goal) {
            const Fraction d = goal - score;
            if (d.d == 0) return std::numeric_limits<long long>::max() / 4;
            return std::llabs(d.n / d.d);
        }
    }

    BattleAI::BattleAI()
        : m_rng(std::random_device{}()) {
    }

    void BattleAI::Reset() {
        m_stayCount1P = 0;
        m_stayCount2P = 0;
    }

    int BattleAI::ChooseRoundStartNumber() {
        std::uniform_int_distribution<int> numberDist(1, 9);
        return numberDist(m_rng);
    }

    char BattleAI::ChooseRoundOperator(
        const std::array<bool, 4>& available,
        int myNumber,
        int enemyNumber,
        const Fraction& myScore,
        int roundTarget,
        const BattleRule& rule) {

        std::uniform_int_distribution<int> noise(-60, 60);
        const int current = static_cast<int>(myScore.n / myScore.d);
        const int remaining = roundTarget - current;

        int bestScore = std::numeric_limits<int>::min();
        char bestOp = '\0';

        for (int i = 0; i < static_cast<int>(OPS.size()); ++i) {
            if (!available[i]) continue;

            const char op = OPS[i];
            const BattleCalculationResult calc =
                rule.CalculateRoundArithmeticResult(myNumber, enemyNumber, op);

            int eval = noise(m_rng);
            if (!calc.valid) {
                eval -= 5000;
            }
            else {
                const int value = calc.normalizedValue;
                if (value == remaining) eval += 1000000;
                else if (value < remaining) eval += value * 250;
                else eval -= (value - remaining) * 350;
                if (op == '/') eval += 120;
            }

            if (bestOp == '\0' || eval > bestScore) {
                bestScore = eval;
                bestOp = op;
            }
        }

        if (bestOp == '\0') {
            for (int i = 0; i < static_cast<int>(OPS.size()); ++i) {
                if (available[i]) return OPS[i];
            }
        }
        return bestOp;
    }

    int BattleAI::GetStayCount(bool is1P) const {
        return is1P ? m_stayCount1P : m_stayCount2P;
    }

    void BattleAI::UpdateStayCount(bool is1P, const UnitBase& me, const MoveDecision& decision) {
        int& stayCount = is1P ? m_stayCount1P : m_stayCount2P;
        const bool isStay = (decision.target == me.GetGridPos());
        if (isStay && !me.HasWarpNode(decision.target)) ++stayCount;
        else stayCount = 0;
    }

    BattleAI::MoveDecision BattleAI::ChooseMove(const MoveContext& context) {
        MoveDecision decision;
        if (!context.map || !context.me || !context.enemy || !context.rule || context.candidates.empty()) {
            return decision;
        }

        int bestScore = -9999999;
        std::uniform_int_distribution<int> noiseDist(-500, 500);

        for (const MoveCandidate& candidate : context.candidates) {
            int eval = EvaluateBoard(context, candidate);
            eval += noiseDist(m_rng);

            if (!decision.valid || eval > bestScore) {
                bestScore = eval;
                decision.valid = true;
                decision.target = candidate.target;
                decision.cost = candidate.cost;
            }
        }

        if (decision.valid) UpdateStayCount(context.is1P, *context.me, decision);
        return decision;
    }

    BattleAI::ActionTarget BattleAI::ChooseActionTarget(const ActionContext& context) const {
        if (!context.me || !context.enemy || !context.rule) return ActionTarget::NONE;

        const UnitBase& me = *context.me;
        const UnitBase& enemy = *context.enemy;
        const BattleRule& rule = *context.rule;

        if (!rule.IsAdjacent(me.GetGridPos(), enemy.GetGridPos()) || me.GetOp() == '\0') {
            return ActionTarget::NONE;
        }

        const int myNumber = me.GetNumber();
        const int enemyNumber = enemy.GetNumber();
        const char op = me.GetOp();
        const BattleCalculationResult calc = rule.CalculateBattleResult(myNumber, enemyNumber, op);

        if (op == '/' && !calc.cleanDivide) return ActionTarget::NONE;

        if (rule.IsRoundBattle()) {
            const Fraction goal(context.roundTarget);
            const Fraction myScoreNow = context.is1P ? context.p1Score : context.p2Score;
            const Fraction enemyScoreNow = context.is1P ? context.p2Score : context.p1Score;

            const BattleSimulatedUnitState selfState = rule.SimulateRoundBattleResult(
                myNumber, me.GetStocks(), myScoreNow, context.roundTarget,
                calc.intValue, calc.fraction, op, calc.cleanDivide);
            const BattleSimulatedUnitState enemyState = rule.SimulateRoundBattleResult(
                enemyNumber, enemy.GetStocks(), enemyScoreNow, context.roundTarget,
                calc.intValue, calc.fraction, op, calc.cleanDivide);

            int scoreSelf = -1000000;
            int scoreEnemy = -1000000;
            if (selfState.valid) {
                const long long before = DistanceToGoal(myScoreNow, goal);
                const long long after = DistanceToGoal(selfState.score, goal);
                scoreSelf = static_cast<int>((before - after) * 300);
                if (selfState.score == goal) scoreSelf += 10000000;
            }
            if (enemyState.valid) {
                const long long before = DistanceToGoal(enemyScoreNow, goal);
                const long long after = DistanceToGoal(enemyState.score, goal);
                // 相手をTARGETへ近づけるほど悪い。TARGETへ入れるのは最悪。
                scoreEnemy = static_cast<int>((after - before) * 300);
                if (enemyState.score == goal) scoreEnemy -= 10000000;
            }
            return scoreSelf >= scoreEnemy ? ActionTarget::SELF : ActionTarget::OPPONENT;
        }

        // 既存モードの挙動維持。
        if (op == '/') return ActionTarget::SELF;

        bool targetSelfIsBetter = true;
        if (rule.IsZeroOne()) {
            const Fraction goal(rule.GetTargetScore());
            const Fraction myScoreNow = context.is1P ? context.p1Score : context.p2Score;
            const Fraction enemyScoreNow = context.is1P ? context.p2Score : context.p1Score;

            const Fraction nextMy = rule.CalculateBouncedScore(myScoreNow, calc.fraction);
            const Fraction nextEnemy = rule.CalculateBouncedScore(enemyScoreNow, calc.fraction);

            const long long myDistNow = DistanceToGoal(myScoreNow, goal);
            const long long enemyDistNow = DistanceToGoal(enemyScoreNow, goal);
            const long long myDistNext = DistanceToGoal(nextMy, goal);
            const long long enemyDistNext = DistanceToGoal(nextEnemy, goal);

            int scoreSelf = static_cast<int>((myDistNow - myDistNext) * 100);
            if (nextMy == goal) scoreSelf = 1000000;

            int scoreEnemy = static_cast<int>((enemyDistNext - enemyDistNow) * 100);
            if (nextEnemy == goal) scoreEnemy = -1000000;

            targetSelfIsBetter = (scoreSelf >= scoreEnemy);
        }
        else {
            const BattleSimulatedUnitState selfState = rule.SimulateClassicResult(
                myNumber, me.GetStocks(), me.GetMaxStocks(), calc.intValue, op, calc.cleanDivide);
            const BattleSimulatedUnitState enemyState = rule.SimulateClassicResult(
                enemyNumber, enemy.GetStocks(), enemy.GetMaxStocks(), calc.intValue, op, calc.cleanDivide);

            int scoreSelf =
                (selfState.stocks - me.GetStocks()) * 10000 +
                (selfState.number - myNumber) * 1000;
            if (selfState.defeated) scoreSelf = -1000000;

            int scoreEnemy =
                (enemy.GetStocks() - enemyState.stocks) * 15000 +
                (enemyNumber - enemyState.number) * 500;
            if (enemyState.defeated) scoreEnemy = 10000000;

            targetSelfIsBetter = (scoreSelf >= scoreEnemy);
        }

        return targetSelfIsBetter ? ActionTarget::SELF : ActionTarget::OPPONENT;
    }

    int BattleAI::EvaluateBoard(const MoveContext& context, const MoveCandidate& candidate) const {
        const UnitBase& me = *context.me;
        const UnitBase& enemy = *context.enemy;
        const MapGrid& map = *context.map;
        const BattleRule& rule = *context.rule;

        int score = 0;
        const IntVector2 currentPos = me.GetGridPos();
        const IntVector2 targetPos = candidate.target;
        const bool isStay = (targetPos == currentPos);
        const IntVector2 enemyPos = enemy.GetGridPos();
        const bool canAttack = rule.IsAdjacent(targetPos, enemyPos);

        const BattleSimulatedUnitState moveState = rule.IsRoundBattle()
            ? rule.SimulateRoundMoveCost(me.GetNumber(), me.GetStocks(), candidate.cost)
            : rule.SimulateMoveCost(me.GetNumber(), me.GetStocks(), me.GetMaxStocks(), candidate.cost);

        const int predictedStocks = moveState.stocks;
        const int predictedPower = moveState.number;
        if (moveState.defeated) score -= 9000000;

        if (isStay) {
            score -= GetStayCount(context.is1P) * 20000;
            if (!canAttack) score -= 2000;
        }

        if (targetPos.x == 0 || targetPos.x == 8 || targetPos.y == 0 || targetPos.y == 8) score -= 50;

        char virtualOp = me.GetOp();
        const char itemHere = map.GetItemAt(targetPos.x, targetPos.y);
        if (itemHere != '\0') virtualOp = itemHere;

        if (rule.IsRoundBattle()) {
            const Fraction goal(context.roundTarget);
            const Fraction myScoreNow = context.is1P ? context.p1Score : context.p2Score;
            const Fraction enemyScoreNow = context.is1P ? context.p2Score : context.p1Score;
            const long long myDistNow = DistanceToGoal(myScoreNow, goal);
            const long long enemyDistNow = DistanceToGoal(enemyScoreNow, goal);

            const int chipHere = map.GetNumberChipAt(targetPos.x, targetPos.y);
            if (chipHere > 0 && virtualOp != '\0') {
                const BattleCalculationResult chipCalc =
                    rule.CalculateRoundArithmeticResult(predictedPower, chipHere, virtualOp);
                if (chipCalc.valid) {
                    const int currentTotal = static_cast<int>(myScoreNow.n / myScoreNow.d);
                    const int remaining = context.roundTarget - currentTotal;
                    score += 2500;
                    if (chipCalc.normalizedValue == remaining) score += 9000000;
                    else if (chipCalc.normalizedValue <= remaining) {
                        score += chipCalc.normalizedValue * 250;
                    }
                }
            }

            if (me.GetOp() == '\0') {
                int bestItemScore = -99999;
                for (int ix = 0; ix < map.GetWidth(); ++ix) {
                    for (int iy = 0; iy < map.GetHeight(); ++iy) {
                        const char item = map.GetItemAt(ix, iy);
                        if (item == '\0') continue;
                        const int distToItem = std::abs(targetPos.x - ix) + std::abs(targetPos.y - iy);
                        int itemValue = (20 - distToItem) * 450;
                        const BattleCalculationResult calc = rule.CalculateRoundArithmeticResult(predictedPower, enemy.GetNumber(), item);
                        if (!calc.valid) itemValue -= 1200;
                        bestItemScore = std::max(bestItemScore, itemValue);
                    }
                }
                if (bestItemScore != -99999) score += bestItemScore;
            }

            if (virtualOp != '\0') {
                const int distToEnemy = std::abs(targetPos.x - enemyPos.x) + std::abs(targetPos.y - enemyPos.y);
                score += (20 - distToEnemy) * 900;

                if (canAttack) {
                    const BattleCalculationResult calc = rule.CalculateRoundArithmeticResult(predictedPower, enemy.GetNumber(), virtualOp);
                    if (calc.valid) {
                        const BattleSimulatedUnitState selfState = rule.SimulateRoundBattleResult(
                            predictedPower, predictedStocks, myScoreNow, context.roundTarget,
                            calc.intValue, calc.fraction, virtualOp, calc.cleanDivide);
                        const BattleSimulatedUnitState enemyState = rule.SimulateRoundBattleResult(
                            enemy.GetNumber(), enemy.GetStocks(), enemyScoreNow, context.roundTarget,
                            calc.intValue, calc.fraction, virtualOp, calc.cleanDivide);

                        if (selfState.valid) {
                            const long long nextDist = DistanceToGoal(selfState.score, goal);
                            int gain = static_cast<int>((myDistNow - nextDist) * 300);
                            if (selfState.score == goal) gain += 10000000;
                            score += gain;
                        }
                        if (enemyState.valid) {
                            const long long nextDist = DistanceToGoal(enemyState.score, goal);
                            int deny = static_cast<int>((nextDist - enemyDistNow) * 250);
                            if (enemyState.score == goal) deny -= 10000000;
                            score += std::max(0, deny);
                        }
                    }
                    else {
                        score -= 1500;
                    }
                }
            }

            if (canAttack && virtualOp == '\0' && enemy.GetOp() != '\0') score -= 5000;
            return score;
        }

        if (rule.IsZeroOne()) {
            const Fraction goal(rule.GetTargetScore());
            const Fraction myScoreNow = context.is1P ? context.p1Score : context.p2Score;
            const Fraction enemyScoreNow = context.is1P ? context.p2Score : context.p1Score;
            const long long myDistNow = DistanceToGoal(myScoreNow, goal);
            const long long enemyDistNow = DistanceToGoal(enemyScoreNow, goal);

            if (me.GetOp() == '\0') {
                int bestItemScore = -99999;
                for (int ix = 0; ix < map.GetWidth(); ++ix) {
                    for (int iy = 0; iy < map.GetHeight(); ++iy) {
                        const char item = map.GetItemAt(ix, iy);
                        if (item == '\0') continue;

                        const int distToItem = std::abs(targetPos.x - ix) + std::abs(targetPos.y - iy);
                        int itemValue = (20 - distToItem) * 300;
                        const BattleCalculationResult calc = rule.CalculateBattleResult(predictedPower, enemy.GetNumber(), item);

                        if (item == '/') {
                            if (calc.cleanDivide) itemValue += 1500;
                            else {
                                bestItemScore = std::max(bestItemScore, itemValue);
                                continue;
                            }
                        }

                        const Fraction nextMy = rule.CalculateBouncedScore(myScoreNow, calc.fraction);
                        const long long myDistNext = DistanceToGoal(nextMy, goal);
                        int benefitSelf = static_cast<int>((myDistNow - myDistNext) * 200);
                        if (nextMy == goal) benefitSelf = 100000;

                        const Fraction nextEnemy = rule.CalculateBouncedScore(enemyScoreNow, calc.fraction);
                        const long long enemyDistNext = DistanceToGoal(nextEnemy, goal);
                        int benefitEnemy = static_cast<int>((enemyDistNext - enemyDistNow) * 200);
                        if (nextEnemy == goal) benefitEnemy = -100000;

                        itemValue += std::max(benefitSelf, benefitEnemy);
                        bestItemScore = std::max(bestItemScore, itemValue);
                    }
                }
                if (bestItemScore != -99999) score += bestItemScore;
            }

            if (virtualOp != '\0') {
                const int distToEnemy = std::abs(targetPos.x - enemyPos.x) + std::abs(targetPos.y - enemyPos.y);
                score += (20 - distToEnemy) * 500;

                if (canAttack) {
                    const BattleCalculationResult calc = rule.CalculateBattleResult(predictedPower, enemy.GetNumber(), virtualOp);
                    if (virtualOp != '/' || calc.cleanDivide) {
                        const Fraction nextMy = rule.CalculateBouncedScore(myScoreNow, calc.fraction);
                        const Fraction nextEnemy = rule.CalculateBouncedScore(enemyScoreNow, calc.fraction);
                        const long long myDistNext = DistanceToGoal(nextMy, goal);
                        const long long enemyDistNext = DistanceToGoal(nextEnemy, goal);

                        int gainSelf = static_cast<int>((myDistNow - myDistNext) * 200);
                        if (nextMy == goal) gainSelf = 1000000;
                        int gainEnemy = static_cast<int>((enemyDistNext - enemyDistNow) * 200);
                        if (nextEnemy == goal) gainEnemy = -1000000;
                        score += std::max(gainSelf, gainEnemy);
                    }
                    if (virtualOp == '/' && calc.cleanDivide) score += 2000;
                }
            }

            if (canAttack && virtualOp == '\0' && enemy.GetOp() != '\0') score -= 5000;
            return score;
        }

        // Classic
        score += predictedStocks * 20000;
        score -= enemy.GetStocks() * 20000;
        score += predictedPower * 100;

        if (me.GetOp() == '\0') {
            int bestItemScore = -99999;
            for (int ix = 0; ix < map.GetWidth(); ++ix) {
                for (int iy = 0; iy < map.GetHeight(); ++iy) {
                    const char item = map.GetItemAt(ix, iy);
                    if (item == '\0') continue;

                    const int distToItem = std::abs(targetPos.x - ix) + std::abs(targetPos.y - iy);
                    int itemValue = (20 - distToItem) * 500;
                    if (item == '-') itemValue += 5000;
                    else if (item == '*') itemValue += 3000;
                    if (predictedStocks == 0 && predictedPower <= 3 && item == '+') itemValue += 8000;
                    bestItemScore = std::max(bestItemScore, itemValue);
                }
            }
            if (bestItemScore != -99999) score += bestItemScore;
        }

        if (virtualOp != '\0') {
            const int distToEnemy = std::abs(targetPos.x - enemyPos.x) + std::abs(targetPos.y - enemyPos.y);
            score += (20 - distToEnemy) * 1000;

            if (canAttack) {
                const BattleCalculationResult calc = rule.CalculateBattleResult(predictedPower, enemy.GetNumber(), virtualOp);
                if (virtualOp == '/') {
                    score += calc.cleanDivide ? 2000 : -1000;
                }
                else {
                    const BattleSimulatedUnitState selfState = rule.SimulateClassicResult(
                        predictedPower, predictedStocks, me.GetMaxStocks(), calc.intValue, virtualOp, calc.cleanDivide);
                    const BattleSimulatedUnitState enemyState = rule.SimulateClassicResult(
                        enemy.GetNumber(), enemy.GetStocks(), enemy.GetMaxStocks(), calc.intValue, virtualOp, calc.cleanDivide);

                    int scoreSelf =
                        (selfState.stocks - predictedStocks) * 10000 +
                        (selfState.number - predictedPower) * 1000;
                    if (selfState.defeated) scoreSelf -= 5000000;

                    int scoreEnemy =
                        (enemy.GetStocks() - enemyState.stocks) * 15000 +
                        (enemy.GetNumber() - enemyState.number) * 500;
                    if (enemyState.defeated) scoreEnemy += 10000000;

                    score += std::max(scoreSelf, scoreEnemy);
                }
            }
        }

        if (canAttack && virtualOp == '\0' && enemy.GetOp() != '\0') score -= 5000;
        return score;
    }

} // namespace App
