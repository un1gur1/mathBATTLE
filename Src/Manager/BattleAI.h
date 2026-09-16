#pragma once

#include <array>
#include <random>
#include <vector>

#include "BattleRule.h"
#include "../Common/Vector2.h"

namespace App {

    class MapGrid;
    class UnitBase;

    class BattleAI {
    public:
        enum class ActionTarget { NONE, SELF, OPPONENT };

        struct MoveCandidate {
            IntVector2 target{ -1, -1 };
            int cost = 1;
        };

        struct MoveContext {
            const MapGrid* map = nullptr;
            const UnitBase* me = nullptr;
            const UnitBase* enemy = nullptr;
            const BattleRule* rule = nullptr;
            bool is1P = false;
            Fraction p1Score{};
            Fraction p2Score{};
            int roundTarget = 0;
            std::vector<MoveCandidate> candidates;
        };

        struct MoveDecision {
            bool valid = false;
            IntVector2 target{ -1, -1 };
            int cost = 1;
        };

        struct ActionContext {
            const UnitBase* me = nullptr;
            const UnitBase* enemy = nullptr;
            const BattleRule* rule = nullptr;
            bool is1P = false;
            Fraction p1Score{};
            Fraction p2Score{};
            int roundTarget = 0;
        };

        BattleAI();

        void Reset();
        MoveDecision ChooseMove(const MoveContext& context);
        ActionTarget ChooseActionTarget(const ActionContext& context) const;
        int ChooseRoundStartNumber();
        char ChooseRoundOperator(
            const std::array<bool, 4>& available,
            int myNumber,
            int enemyNumber,
            const Fraction& myScore,
            int roundTarget,
            const BattleRule& rule);

    private:
        int EvaluateBoard(const MoveContext& context, const MoveCandidate& candidate) const;
        int GetStayCount(bool is1P) const;
        void UpdateStayCount(bool is1P, const UnitBase& me, const MoveDecision& decision);

        int m_stayCount1P = 0;
        int m_stayCount2P = 0;
        std::mt19937 m_rng;
    };

} // namespace App
