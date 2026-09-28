#define NOMINMAX
#include "BattleMaster.h"
#include <DxLib.h>
#include <cmath>
#include <algorithm>
#include "../Input/InputManager.h"
#include "../Scene/SceneManager.h"
#include "NetworkManager.h"
#include "BattleAI.h"
#include "../Battle/BattleUI.h"
#include "ProceduralAudio.h"

namespace {
    constexpr int SCREEN_W = 1920;
}

namespace App {

    BattleMaster::BattleMaster()
        : m_currentPhase(Phase::P1_Move)
        , m_gameMode(GameMode::VS_CPU)
        , m_mapGrid(80, Vector2(600, 120))
        , m_p1ZeroOneScore(0, 1), m_p2ZeroOneScore(0, 1)
        , m_isPlayerSelected(false)
        , m_hoverGrid(-1, -1)
        , m_enemyAIStarted(false)
        , m_playerAIStarted(false)
        , m_is1P_NPC(false)
        , m_is2P_NPC(true)
    {
    }

    BattleMaster::~BattleMaster() = default;

    void BattleMaster::AddLog(const std::string& message) {
        if (m_ui) {
            m_ui->AddLog(message);
        }
    }

    void BattleMaster::SetClassicDefeat(UnitBase& loser, const std::string& reason) {
        if (m_isBattleFinished) return;

        std::string loserName = (&loser == m_player.get()) ? "1P" : "2P";

        m_isBattleFinished = true;
        m_is1PWinner = (&loser == m_enemy.get());

        AddLog("【決着】 " + loserName + " は " + reason + " により、予備バッテリーを使い切って戦闘不能！");
    }

    bool BattleMaster::Is1PTurn() const {
        return (m_currentPhase == Phase::P1_TurnStart || m_currentPhase == Phase::P1_Move || m_currentPhase == Phase::P1_Action);
    }

    UnitBase* BattleMaster::GetActiveUnit()const {
        return Is1PTurn() ? static_cast<UnitBase*>(m_player.get()) : static_cast<UnitBase*>(m_enemy.get());
    }

    UnitBase* BattleMaster::GetTargetUnit() const {
        return Is1PTurn() ? static_cast<UnitBase*>(m_enemy.get()) : static_cast<UnitBase*>(m_player.get());
    }

    UnitBase* BattleMaster::GetUnitBySide(bool is1P) const {
        return is1P ? static_cast<UnitBase*>(m_player.get()) : static_cast<UnitBase*>(m_enemy.get());
    }

    void BattleMaster::ReserveOperatorUpkeepIfNeeded(UnitBase& unit, bool is1P) {
        if (!m_rule.UsesOperatorUpkeep()) return;
        if (unit.IsMoving()) return;
        if (unit.GetOp() == '\0') return;

        bool& pending = is1P ? m_p1OpCostPending : m_p2OpCostPending;
        if (pending) return;

        pending = true;
        AddLog(std::string("【警告】 ") + (is1P ? "1P" : "2P") +
            ": 演算子負荷を検知。ターン終了時に保持中ならバッテリーが 1 減少します。");
    }

    void BattleMaster::ApplyOperatorUpkeepCost(bool is1P) {
        if (!m_rule.UsesOperatorUpkeep()) return;

        bool& pending = is1P ? m_p1OpCostPending : m_p2OpCostPending;
        if (!pending) return;

        UnitBase* unit = GetUnitBySide(is1P);
        if (unit && unit->GetOp() != '\0') {
            if (unit->GetStocks() <= 0) {
                SetClassicDefeat(*unit, "演算子の過負荷");
                ProceduralAudio::GetInstance().PlayErrorSE();
            }
            else {
                unit->AddStocks(-1);
                AddLog(std::string("【負荷】 ") + (is1P ? "1P" : "2P") +
                    ": 演算子を保持していたため、バッテリーが 1 減少しました。");
                ProceduralAudio::GetInstance().PlayErrorSE();
            }
        }
        pending = false;
    }

    void BattleMaster::FinishActionPhase(bool is1P) {
        ApplyOperatorUpkeepCost(is1P);
        if (m_rule.IsRoundBattle()) ConsumeRoundSubIfUsed(is1P);

        // ラウンド勝利が確定した直後は次ターンへ進めない。
        if (m_rule.IsRoundBattle() && m_roundPhase != RoundPhase::BATTLE) return;
        if (IsGameOver()) return;

        // ラウンドバトルの演算子は各ラウンド開始時にドラフト/配置するため、
        // 旧モード用のターン経過リスポーンは行わない。
        if (!is1P && !m_rule.IsRoundBattle()) {
            m_mapGrid.UpdateTurn();
        }

        m_currentPhase = is1P ? Phase::P2_TurnStart : Phase::P1_TurnStart;
        m_turnStartTimer = 40;
        m_aiWaitTimer = 35;
    }

    int BattleMaster::ReadRoundNumberKey() const {
        auto& input = InputManager::GetInstance();
        for (int i = 0; i < 9; ++i) {
            if (input.IsTrgDown(KEY_INPUT_1 + i) || input.IsTrgDown(KEY_INPUT_NUMPAD1 + i)) {
                return i + 1;
            }
        }
        return 0;
    }

    int BattleMaster::ReadRoundOperatorKey() const {
        auto& input = InputManager::GetInstance();
        for (int i = 0; i < 4; ++i) {
            if (input.IsTrgDown(KEY_INPUT_1 + i) || input.IsTrgDown(KEY_INPUT_NUMPAD1 + i)) {
                return i;
            }
        }
        return -1;
    }

    bool BattleMaster::IsOnlineBattle() const {
        return NetworkManager::GetInstance() != nullptr &&
            NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED;
    }

    bool BattleMaster::IsLocalRoundController(bool is1P) const {
        if (!IsOnlineBattle()) return true;
        return NetworkManager::GetInstance()->IsHost() == is1P;
    }

    void BattleMaster::SendRoundSelection(NetAction action, int value) const {
        if (!IsOnlineBattle()) return;
        BattlePacket packet;
        packet.actionType = action;
        packet.targetX = value;
        packet.targetY = 0;
        NetworkManager::GetInstance()->SendBattlePacket(packet);
    }

    bool BattleMaster::ReceiveRoundSelection(NetAction action, int& value) const {
        if (!IsOnlineBattle()) return false;
        BattlePacket packet;
        if (!NetworkManager::GetInstance()->ReceiveBattlePacket(packet)) return false;
        if (packet.actionType != action) return false;
        value = packet.targetX;
        return true;
    }

    int BattleMaster::FindRoundOperatorIndex(char op) const {
        for (int i = 0; i < static_cast<int>(m_roundOperators.size()); ++i) {
            if (m_roundOperators[i] == op) return i;
        }
        return -1;
    }

    int BattleMaster::FindNextAvailableRoundOperatorIndex(int from, int direction) const {
        if (direction == 0) direction = 1;
        int index = from;
        for (int step = 0; step < static_cast<int>(m_roundOperators.size()); ++step) {
            index += direction;
            if (index < 0) index = static_cast<int>(m_roundOperators.size()) - 1;
            if (index >= static_cast<int>(m_roundOperators.size())) index = 0;
            if (m_roundOperatorAvailable[index]) return index;
        }
        return from;
    }

    void BattleMaster::ConfirmRoundStartNumber(bool is1P, int number) {
        number = std::clamp(number, 1, 9);

        if (is1P) {
            m_p1RoundStartNumber = number;
            AddLog("【ラウンド】 1P 初期数字: " + std::to_string(number));
            m_roundPhase = RoundPhase::SELECT_P2_NUMBER;
            m_roundNumberCursor = 5;
            m_roundSetupWaitTimer = 20;
        }
        else {
            m_p2RoundStartNumber = number;
            m_roundTarget = m_rule.CalculateRoundTarget(m_p1RoundStartNumber, m_p2RoundStartNumber);
            m_p1RoundScore = Fraction(0);
            m_p2RoundScore = Fraction(0);
            m_p1DisplayScore = 0.0f;
            m_p2DisplayScore = 0.0f;

            if (m_player) m_player->SetNumber(m_p1RoundStartNumber);
            if (m_enemy) m_enemy->SetNumber(m_p2RoundStartNumber);

            AddLog("【ラウンド】 2P 初期数字: " + std::to_string(number));
            AddLog("【目標値】 9 + " + std::to_string(m_p1RoundStartNumber) +
                " + " + std::to_string(m_p2RoundStartNumber) +
                " = " + std::to_string(m_roundTarget));
            m_roundPhase = RoundPhase::TARGET_REVEAL;
            m_roundSetupWaitTimer = 60;
        }

        ProceduralAudio::GetInstance().PlayPowerSE(number);
    }

    void BattleMaster::ConfirmRoundOperator(bool is1P, char op) {
        const int index = FindRoundOperatorIndex(op);
        if (index < 0 || !m_roundOperatorAvailable[index]) return;

        m_roundOperatorAvailable[index] = false;
        if (is1P) {
            m_p1DraftedOperator = op;
            if (m_player) m_player->SetOp(op);
            AddLog("【演算子選択】 1P が [" + std::string(1, op) + "] を選択");
            m_roundPhase = RoundPhase::DRAFT_P2_OPERATOR;
            m_roundOperatorCursor = FindNextAvailableRoundOperatorIndex(index, 1);
            m_roundSetupWaitTimer = 20;
        }
        else {
            m_p2DraftedOperator = op;
            if (m_enemy) m_enemy->SetOp(op);
            AddLog("【演算子選択】 2P が [" + std::string(1, op) + "] を選択");
            PlaceRemainingRoundOperators();
            m_roundPhase = RoundPhase::PLACE_OPERATORS;
            m_roundSetupWaitTimer = 75;
        }

        if (op == '+') ProceduralAudio::GetInstance().PlayPowerSE(5);
        else if (op == '-') ProceduralAudio::GetInstance().PlayPowerSE(2);
        else if (op == '*') ProceduralAudio::GetInstance().PlayPowerSE(7);
        else if (op == '/') ProceduralAudio::GetInstance().PlayPowerSE(9);
    }

    void BattleMaster::PlaceRemainingRoundOperators() {
        std::array<char, 2> remaining{ '\0', '\0' };
        int count = 0;
        for (int i = 0; i < static_cast<int>(m_roundOperators.size()) && count < 2; ++i) {
            if (m_roundOperatorAvailable[i]) remaining[count++] = m_roundOperators[i];
        }

        m_mapGrid.ClearItems();

        // 完全固定配置では初期位置と衝突し得るため、候補順だけ固定し、
        // 駒がいるマスを飛ばして最初の2マスへ置く。ランダム性はない。
        const std::array<IntVector2, 9> candidates{
            IntVector2{ 2, 4 }, IntVector2{ 6, 4 },
            IntVector2{ 4, 2 }, IntVector2{ 4, 6 },
            IntVector2{ 4, 4 }, IntVector2{ 2, 2 },
            IntVector2{ 6, 6 }, IntVector2{ 6, 2 }, IntVector2{ 2, 6 }
        };

        IntVector2 first{ -1, -1 };
        IntVector2 second{ -1, -1 };
        const IntVector2 p1 = m_player ? m_player->GetGridPos() : IntVector2{ -1, -1 };
        const IntVector2 p2 = m_enemy ? m_enemy->GetGridPos() : IntVector2{ -1, -1 };

        for (const IntVector2& pos : candidates) {
            if (pos == p1 || pos == p2) continue;
            if (first.x < 0) first = pos;
            else { second = pos; break; }
        }

        m_roundPlacedOperator1 = remaining[0];
        m_roundPlacedOperator2 = remaining[1];
        m_roundPlacedPos1 = first;
        m_roundPlacedPos2 = second;

        if (m_roundPlacedOperator1 != '\0' && first.x >= 0) {
            m_mapGrid.SetItemAt(first.x, first.y, m_roundPlacedOperator1);
        }
        if (m_roundPlacedOperator2 != '\0' && second.x >= 0) {
            m_mapGrid.SetItemAt(second.x, second.y, m_roundPlacedOperator2);
        }

        AddLog("【盤面配置】 残り演算子 [" + std::string(1, m_roundPlacedOperator1) + "] [" +
            std::string(1, m_roundPlacedOperator2) + "] を盤面へ配置");
    }

    void BattleMaster::BeginRoundBattle() {
        m_roundPhase = RoundPhase::BATTLE;
        m_currentPhase = Phase::P1_TurnStart;
        m_turnStartTimer = 80;
        m_aiWaitTimer = 30;
        m_isPlayerSelected = false;
        m_playerAIStarted = false;
        m_enemyAIStarted = false;
        m_p1OpCostPending = false;
        m_p2OpCostPending = false;

        m_p1SubOperator = '\0';
        m_p2SubOperator = '\0';
        m_p1UsingSub = false;
        m_p2UsingSub = false;
        m_p1TurnOperator = m_p1DraftedOperator;
        m_p2TurnOperator = m_p2DraftedOperator;
        SyncRoundTurnOperator(true);
        SyncRoundTurnOperator(false);

        m_roundResultPending = false;
        m_roundChipPlacementPending = false;

        AddLog("【ラウンド " + std::to_string(m_roundNumber) + "】 バトル開始 / 目標値 " + std::to_string(m_roundTarget));
        AddLog("1P 合計値=0  2P 合計値=0 / 先に2ラウンド取った方が勝利");
        ProceduralAudio::GetInstance().PlayPowerSE(9);
    }

    void BattleMaster::EndRound(int winnerSide) {
        if (!m_rule.IsRoundBattle() || m_roundPhase != RoundPhase::BATTLE) return;
        if (winnerSide != 1 && winnerSide != 2) return;

        m_roundWinner = winnerSide;
        if (winnerSide == 1) ++m_p1RoundWins;
        else ++m_p2RoundWins;

        AddLog("【ラウンド勝利】 " + std::to_string(winnerSide) + "P が目標値にぴったり到達！");
        AddLog("【試合状況】 1P " + std::to_string(m_p1RoundWins) +
            " - " + std::to_string(m_p2RoundWins) + " 2P");

        m_roundPhase = RoundPhase::ROUND_END;
        m_roundSetupWaitTimer = 120;
        m_effectIntensity = 2.5f;
        ProceduralAudio::GetInstance().PlayPowerSE(9);

        if (m_p1RoundWins >= 2 || m_p2RoundWins >= 2) {
            m_isBattleFinished = true;
            m_is1PWinner = (m_p1RoundWins >= 2);
            AddLog("【試合終了】 " + std::to_string(winnerSide) + "P 勝利 / 3ラウンド制2本先取");
        }
    }

    void BattleMaster::ResetRoundBoardState() {
        const int p1Max = m_player ? m_player->GetMaxStocks() : 3;
        const int p2Max = m_enemy ? m_enemy->GetMaxStocks() : 3;

        m_player = std::make_unique<Player>(
            m_p1RoundStartPos,
            m_mapGrid.GetCellCenter(m_p1RoundStartPos.x, m_p1RoundStartPos.y),
            5, p1Max, p1Max);
        m_enemy = std::make_unique<Enemy>(
            m_p2RoundStartPos,
            m_mapGrid.GetCellCenter(m_p2RoundStartPos.x, m_p2RoundStartPos.y),
            5, p2Max, p2Max);

        m_player->SetOp('\0');
        m_enemy->SetOp('\0');
        m_mapGrid.ClearItems();

        m_p1RoundStartNumber = 0;
        m_p2RoundStartNumber = 0;
        m_roundTarget = 0;
        m_roundNumberCursor = 5;
        m_p1RoundScore = Fraction(0);
        m_p2RoundScore = Fraction(0);
        m_p1DisplayScore = 0.0f;
        m_p2DisplayScore = 0.0f;

        m_roundOperatorAvailable = { true, true, true, true };
        m_roundOperatorCursor = 0;
        m_p1DraftedOperator = '\0';
        m_p2DraftedOperator = '\0';
        m_p1SubOperator = '\0';
        m_p2SubOperator = '\0';
        m_p1TurnOperator = '\0';
        m_p2TurnOperator = '\0';
        m_p1UsingSub = false;
        m_p2UsingSub = false;
        m_roundPlacedOperator1 = '\0';
        m_roundPlacedOperator2 = '\0';
        m_roundPlacedPos1 = { -1, -1 };
        m_roundPlacedPos2 = { -1, -1 };
        m_roundWinner = 0;

        m_roundResultPending = false;
        m_roundChipPlacementPending = false;
        m_roundPendingResult = 0;
        m_roundPendingRaw = 0;
        m_roundPendingOperand = 0;
        m_roundPendingOperandWasChip = false;
        m_roundPendingChipSource = { -1, -1 };

        m_currentPhase = Phase::P1_TurnStart;
        m_turnStartTimer = 0;
        m_aiWaitTimer = 30;
        m_isPlayerSelected = false;
        m_playerAIStarted = false;
        m_enemyAIStarted = false;
        if (m_ai) m_ai->Reset();
    }

    void BattleMaster::PrepareNextRound() {
        if (m_isBattleFinished) {
            m_roundPhase = RoundPhase::INACTIVE;
            m_currentPhase = Phase::FINISH;
            m_finishTimer = 0;
            m_effectIntensity = 1.0f;
            return;
        }

        ++m_roundNumber;
        ResetRoundBoardState();
        m_roundPhase = RoundPhase::ROUND_START;
        m_roundSetupWaitTimer = 30;
        AddLog(">>> ラウンド " + std::to_string(m_roundNumber) + " 開始");
    }

    void BattleMaster::UpdateRoundSetup() {
        auto& input = InputManager::GetInstance();
        if (m_roundSetupWaitTimer > 0) --m_roundSetupWaitTimer;

        const bool confirm = input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN);
        const bool back = input.IsTrgDown(KEY_INPUT_B) || input.IsTrgDown(KEY_INPUT_BACK);
        const bool online = IsOnlineBattle();
        const Vector2 mousePos = input.GetMousePos();
        const bool mouseClick = input.IsMouseLeftTrg();

        switch (m_roundPhase) {
        case RoundPhase::ROUND_START:
        {
            const bool startClicked = mouseClick &&
                CheckButtonClick(760, 720, 400, 70, mousePos);
            if (((confirm || startClicked) && !online && m_roundSetupWaitTimer <= 0) ||
                (m_roundSetupWaitTimer <= 0 && (online || (m_is1P_NPC && m_is2P_NPC)))) {
                m_roundPhase = RoundPhase::SELECT_P1_NUMBER;
                m_roundNumberCursor = 5;
                m_roundSetupWaitTimer = 10;
                ProceduralAudio::GetInstance().PlayPowerSE(9);
            }
            break;
        }

        case RoundPhase::SELECT_P1_NUMBER:
        case RoundPhase::SELECT_P2_NUMBER:
        {
            const bool is1P = (m_roundPhase == RoundPhase::SELECT_P1_NUMBER);
            const bool localController = IsLocalRoundController(is1P);
            const bool isNPC = !online && (is1P ? m_is1P_NPC : m_is2P_NPC);

            if (!localController) {
                int remoteValue = 0;
                if (ReceiveRoundSelection(NetAction::ROUND_NUMBER, remoteValue)) {
                    ConfirmRoundStartNumber(is1P, remoteValue);
                }
                break;
            }

            if (isNPC) {
                if (m_roundSetupWaitTimer <= 0 && m_ai) {
                    ConfirmRoundStartNumber(is1P, m_ai->ChooseRoundStartNumber());
                }
                break;
            }

            if (mouseClick && m_roundSetupWaitTimer <= 0) {
                constexpr int BOX_W = 86;
                constexpr int BOX_H = 86;
                constexpr int GAP = 16;
                constexpr int COUNT = 9;
                const int totalW = BOX_W * COUNT + GAP * (COUNT - 1);
                const int startX = (1920 - totalW) / 2;
                const int y = 465;

                for (int i = 0; i < COUNT; ++i) {
                    const int x = startX + i * (BOX_W + GAP);
                    if (CheckButtonClick(x, y, BOX_W, BOX_H, mousePos)) {
                        m_roundNumberCursor = i + 1;
                        if (online) SendRoundSelection(NetAction::ROUND_NUMBER, m_roundNumberCursor);
                        ConfirmRoundStartNumber(is1P, m_roundNumberCursor);
                        break;
                    }
                }
                if (m_roundPhase != (is1P ? RoundPhase::SELECT_P1_NUMBER : RoundPhase::SELECT_P2_NUMBER)) {
                    break;
                }
            }

            const int directNumber = ReadRoundNumberKey();
            if (directNumber != 0 && directNumber != m_roundNumberCursor) {
                m_roundNumberCursor = directNumber;
                ProceduralAudio::GetInstance().PlayPowerSE(2);
            }
            if (input.IsTrgDown(KEY_INPUT_LEFT) || input.IsTrgDown(KEY_INPUT_A)) {
                --m_roundNumberCursor;
                if (m_roundNumberCursor < 1) m_roundNumberCursor = 9;
                ProceduralAudio::GetInstance().PlayPowerSE(2);
            }
            if (input.IsTrgDown(KEY_INPUT_RIGHT) || input.IsTrgDown(KEY_INPUT_D)) {
                ++m_roundNumberCursor;
                if (m_roundNumberCursor > 9) m_roundNumberCursor = 1;
                ProceduralAudio::GetInstance().PlayPowerSE(2);
            }

            if (!online && back && m_roundSetupWaitTimer <= 0) {
                if (is1P) {
                    m_roundPhase = RoundPhase::ROUND_START;
                }
                else {
                    m_roundPhase = RoundPhase::SELECT_P1_NUMBER;
                    m_roundNumberCursor = m_p1RoundStartNumber > 0 ? m_p1RoundStartNumber : 5;
                    m_p1RoundStartNumber = 0;
                }
                m_roundSetupWaitTimer = 10;
                ProceduralAudio::GetInstance().PlayErrorSE();
                break;
            }

            if (confirm && m_roundSetupWaitTimer <= 0) {
                if (online) SendRoundSelection(NetAction::ROUND_NUMBER, m_roundNumberCursor);
                ConfirmRoundStartNumber(is1P, m_roundNumberCursor);
            }
            break;
        }

        case RoundPhase::TARGET_REVEAL:
            if (m_roundSetupWaitTimer <= 0 || (!online && (confirm || mouseClick))) {
                m_roundPhase = RoundPhase::DRAFT_P1_OPERATOR;
                m_roundOperatorCursor = FindNextAvailableRoundOperatorIndex(-1, 1);
                m_roundSetupWaitTimer = 15;
                ProceduralAudio::GetInstance().PlayPowerSE(9);
            }
            break;

        case RoundPhase::DRAFT_P1_OPERATOR:
        case RoundPhase::DRAFT_P2_OPERATOR:
        {
            const bool is1P = (m_roundPhase == RoundPhase::DRAFT_P1_OPERATOR);
            const bool localController = IsLocalRoundController(is1P);
            const bool isNPC = !online && (is1P ? m_is1P_NPC : m_is2P_NPC);

            if (!localController) {
                int remoteValue = 0;
                if (ReceiveRoundSelection(NetAction::ROUND_OPERATOR, remoteValue)) {
                    ConfirmRoundOperator(is1P, static_cast<char>(remoteValue));
                }
                break;
            }

            if (isNPC) {
                if (m_roundSetupWaitTimer <= 0 && m_ai) {
                    const Fraction myScore = is1P ? m_p1RoundScore : m_p2RoundScore;
                    const int myNumber = is1P ? m_p1RoundStartNumber : m_p2RoundStartNumber;
                    const int enemyNumber = is1P ? m_p2RoundStartNumber : m_p1RoundStartNumber;
                    const char chosen = m_ai->ChooseRoundOperator(
                        m_roundOperatorAvailable, myNumber, enemyNumber, myScore, m_roundTarget, m_rule);
                    ConfirmRoundOperator(is1P, chosen);
                }
                break;
            }

            if (mouseClick && m_roundSetupWaitTimer <= 0) {
                constexpr int BOX_W = 160;
                constexpr int BOX_H = 120;
                constexpr int GAP = 42;
                const int totalW = BOX_W * 4 + GAP * 3;
                const int startX = (1920 - totalW) / 2;
                const int y = 490;

                for (int i = 0; i < 4; ++i) {
                    const int x = startX + i * (BOX_W + GAP);
                    if (m_roundOperatorAvailable[i] &&
                        CheckButtonClick(x, y, BOX_W, BOX_H, mousePos)) {
                        m_roundOperatorCursor = i;
                        const char op = m_roundOperators[i];
                        if (online) SendRoundSelection(NetAction::ROUND_OPERATOR, static_cast<int>(op));
                        ConfirmRoundOperator(is1P, op);
                        break;
                    }
                }
                if (m_roundPhase != (is1P ? RoundPhase::DRAFT_P1_OPERATOR : RoundPhase::DRAFT_P2_OPERATOR)) {
                    break;
                }
            }

            const int directIndex = ReadRoundOperatorKey();
            if (directIndex >= 0 && m_roundOperatorAvailable[directIndex]) {
                m_roundOperatorCursor = directIndex;
            }
            if (input.IsTrgDown(KEY_INPUT_LEFT) || input.IsTrgDown(KEY_INPUT_A)) {
                m_roundOperatorCursor = FindNextAvailableRoundOperatorIndex(m_roundOperatorCursor, -1);
                ProceduralAudio::GetInstance().PlayPowerSE(2);
            }
            if (input.IsTrgDown(KEY_INPUT_RIGHT) || input.IsTrgDown(KEY_INPUT_D)) {
                m_roundOperatorCursor = FindNextAvailableRoundOperatorIndex(m_roundOperatorCursor, 1);
                ProceduralAudio::GetInstance().PlayPowerSE(2);
            }

            if (!online && back && m_roundSetupWaitTimer <= 0) {
                if (is1P) {
                    m_roundPhase = RoundPhase::TARGET_REVEAL;
                    m_roundSetupWaitTimer = 10;
                }
                else {
                    const int p1Index = FindRoundOperatorIndex(m_p1DraftedOperator);
                    if (p1Index >= 0) m_roundOperatorAvailable[p1Index] = true;
                    m_p1DraftedOperator = '\0';
                    if (m_player) m_player->SetOp('\0');
                    m_roundPhase = RoundPhase::DRAFT_P1_OPERATOR;
                    m_roundOperatorCursor = FindNextAvailableRoundOperatorIndex(-1, 1);
                    m_roundSetupWaitTimer = 10;
                }
                ProceduralAudio::GetInstance().PlayErrorSE();
                break;
            }

            if (confirm && m_roundSetupWaitTimer <= 0 &&
                m_roundOperatorCursor >= 0 && m_roundOperatorCursor < 4 &&
                m_roundOperatorAvailable[m_roundOperatorCursor]) {
                const char op = m_roundOperators[m_roundOperatorCursor];
                if (online) SendRoundSelection(NetAction::ROUND_OPERATOR, static_cast<int>(op));
                ConfirmRoundOperator(is1P, op);
            }
            break;
        }

        case RoundPhase::PLACE_OPERATORS:
            if (m_roundSetupWaitTimer <= 0 || (!online && (confirm || mouseClick))) {
                BeginRoundBattle();
            }
            break;

        case RoundPhase::ROUND_END:
            if (m_roundSetupWaitTimer <= 0 || (!online && (confirm || mouseClick))) {
                PrepareNextRound();
            }
            break;

        default:
            break;
        }
    }

    void BattleMaster::Init() {
        auto* sm = SceneManager::GetInstance();
        m_gameMode = (sm->GetPlayerCount() == 1) ? GameMode::VS_CPU : GameMode::VS_PLAYER;
        BattleRuleKind ruleKind = BattleRuleKind::CLASSIC;
        if (sm->GetGameMode() == 1) ruleKind = BattleRuleKind::ZERO_ONE;
        else if (sm->GetGameMode() == 2) ruleKind = BattleRuleKind::ROUND_BATTLE;
        m_rule.Configure(ruleKind, sm->GetZeroOneScore());

        // ★ 完全修正版：OFFLINE以外ならオンライン対戦とみなす ★
        // （Initの時点では接続待機中のためCONNECTEDにならないことを考慮）
        bool isOnline = false;
        if (NetworkManager::GetInstance() != nullptr) {
            if (NetworkManager::GetInstance()->GetState() != NetworkManager::State::OFFLINE) {
                isOnline = true;
            }
        }

        m_is1P_NPC = sm->Is1PNPC();
        m_is2P_NPC = sm->Is2PNPC();

        if (isOnline) {
            m_is1P_NPC = false;
            m_is2P_NPC = false;
        }

        int maxStocks = sm->GetMaxStocks();
        m_p1ZeroOneScore = Fraction(0, 1);
        m_p2ZeroOneScore = Fraction(0, 1);

        m_p1DisplayScore = 0.0f;
        m_p2DisplayScore = 0.0f;

        IntVector2 p1StartPos{ sm->Get1PStartX(), sm->Get1PStartY() };
        IntVector2 p2StartPos{ sm->Get2PStartX(), sm->Get2PStartY() };
        m_p1RoundStartPos = p1StartPos;
        m_p2RoundStartPos = p2StartPos;

        // MapGridは現時点で旧2モードのみ。ラウンド専用盤面生成は次段階で追加するため、
        // 土台段階ではCLASSIC側の盤面表現を使用する。
        BattleRuleMode mapMode = m_rule.IsZeroOne() ? BattleRuleMode::ZERO_ONE : BattleRuleMode::CLASSIC;

        m_player = std::make_unique<Player>(p1StartPos, m_mapGrid.GetCellCenter(p1StartPos.x, p1StartPos.y), sm->Get1PStartNum(), maxStocks, maxStocks);
        m_enemy = std::make_unique<Enemy>(p2StartPos, m_mapGrid.GetCellCenter(p2StartPos.x, p2StartPos.y), sm->Get2PStartNum(), maxStocks, maxStocks);

        m_player->SetOp('\0');
        m_enemy->SetOp('\0');

        m_currentPhase = Phase::P1_Move;
        m_isPlayerSelected = false;
        m_enemyAIStarted = false;
        m_playerAIStarted = false;
        m_finishTimer = 0;

        m_startTime = GetNowCount();
        m_p1TotalMoves = 0; m_p2TotalMoves = 0;
        m_p1TotalOps = 0;   m_p2TotalOps = 0;
        m_p1MaxDamage = 0;  m_p2MaxDamage = 0;

        m_effectIntensity = 0.0f;

        if (!m_ai) m_ai = std::make_unique<BattleAI>();
        m_ai->Reset();

        m_p1OpCostPending = false;
        m_p2OpCostPending = false;

        m_isBattleFinished = false;
        m_is1PWinner = false;

        m_currentPhase = Phase::P1_TurnStart;
        m_turnStartTimer = 80;
        m_aiWaitTimer = 30;

        m_roundPhase = RoundPhase::INACTIVE;
        m_roundNumber = 0;
        m_p1RoundStartNumber = 0;
        m_p2RoundStartNumber = 0;
        m_roundTarget = 0;
        m_roundNumberCursor = 5;
        m_roundSetupWaitTimer = 0;
        m_p1RoundScore = Fraction(0);
        m_p2RoundScore = Fraction(0);
        m_roundOperatorAvailable = { true, true, true, true };
        m_roundOperatorCursor = 0;
        m_p1DraftedOperator = '\0';
        m_p2DraftedOperator = '\0';
        m_roundPlacedOperator1 = '\0';
        m_roundPlacedOperator2 = '\0';
        m_roundPlacedPos1 = { -1, -1 };
        m_roundPlacedPos2 = { -1, -1 };
        m_roundWinner = 0;
        m_p1RoundWins = 0;
        m_p2RoundWins = 0;
        m_p1SubOperator = '\0';
        m_p2SubOperator = '\0';
        m_p1TurnOperator = '\0';
        m_p2TurnOperator = '\0';
        m_p1UsingSub = false;
        m_p2UsingSub = false;
        m_roundResultPending = false;
        m_roundChipPlacementPending = false;
        m_roundPendingIs1P = true;
        m_roundPendingResult = 0;
        m_roundPendingRaw = 0;
        m_roundPendingOperand = 0;
        m_roundPendingOperandWasChip = false;
        m_roundPendingChipSource = { -1, -1 };
        if (m_rule.IsRoundBattle()) {
            m_roundPhase = RoundPhase::ROUND_START;
            m_roundNumber = 1;
            m_roundSetupWaitTimer = 15;
            m_turnStartTimer = 0;
        }

        m_ui = std::make_unique<BattleUI>();
        m_ui->Init();
        int stageIdx = sm->GetStageIndex();
        m_mapGrid.SetRuleModeAndStage(mapMode, stageIdx);
        if (m_rule.IsRoundBattle()) {
            // ラウンド用演算子はドラフト後に2個だけ配置する。
            m_mapGrid.ClearItems();
        }
        std::string rModeStr = m_rule.IsClassic() ? "ノーマル" : (m_rule.IsZeroOne() ? "カウント" : "ラウンド");
        AddLog(">>> バトル開始！ [1P:" + std::string(m_is1P_NPC ? "コンピューター" : "プレイヤー") + " vs 2P:" + std::string(m_is2P_NPC ? "コンピューター" : "プレイヤー") + " / " + rModeStr + "]");

        if (m_rule.IsZeroOne()) {
            AddLog(">>> 目標：相手よりはやくスコアを【 " + std::to_string(m_rule.GetTargetScore()) + " 】にしよう！");
        }
        else if (m_rule.IsRoundBattle()) {
            AddLog(">>> ラウンド1開始：ラウンド固有の数字・演算子選択を行います。");
        }
        else {
            AddLog(">>> 目標：相手の残機をなくして、勝利を目指そう！");
        }
    }

    bool BattleMaster::CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const {
        const UnitBase* unit = nullptr;
        if (m_player && m_player->GetGridPos() == start) unit = m_player.get();
        else if (m_enemy && m_enemy->GetGridPos() == start) unit = m_enemy.get();

        const bool isWarpNode = unit && unit->HasWarpNode(target);
        if (!m_rule.CanMove(number, op, start, target, isWarpNode, outCost)) return false;

        if (m_rule.IsRoundBattle()) {
            const IntVector2 other = (unit == m_player.get() && m_enemy)
                ? m_enemy->GetGridPos()
                : ((unit == m_enemy.get() && m_player) ? m_player->GetGridPos() : IntVector2{ -99, -99 });
            if (target == other) return false;
            if (!CanRoundMovePath(start, target, op, isWarpNode)) return false;
        }
        return true;
    }

    bool BattleMaster::IsOccupiedByUnit(IntVector2 pos) const {
        return (m_player && m_player->GetGridPos() == pos) ||
            (m_enemy && m_enemy->GetGridPos() == pos);
    }

    bool BattleMaster::CanRoundMovePath(
        IntVector2 start,
        IntVector2 target,
        char op,
        bool isWarp) const {

        if (isWarp) return true;

        const int dx = target.x - start.x;
        const int dy = target.y - start.y;

        // / の縦2マスジャンプは中間マスを通過しない。
        if (op == '/' && dx == 0 && std::abs(dy) == 2) return true;

        const int stepX = (dx > 0) ? 1 : (dx < 0 ? -1 : 0);
        const int stepY = (dy > 0) ? 1 : (dy < 0 ? -1 : 0);
        const int steps = std::max(std::abs(dx), std::abs(dy));

        IntVector2 cur = start;
        for (int i = 1; i <= steps; ++i) {
            cur.x += stepX;
            cur.y += stepY;

            const bool isTarget = (cur == target);

            // 相手駒は通過も着地も不可。
            if (IsOccupiedByUnit(cur)) {
                return false;
            }

            // 通常移動では最初のアイテムマスまでは移動可能。
            // そのアイテムより先には進めない。
            if (m_mapGrid.HasAnyItemAt(cur.x, cur.y)) {
                return isTarget;
            }
        }
        return true;
    }

    void BattleMaster::SyncRoundTurnOperator(bool is1P) {
        UnitBase* unit = GetUnitBySide(is1P);
        if (!unit) return;
        const char op = is1P ? m_p1TurnOperator : m_p2TurnOperator;
        unit->SetOp(op);
    }

    void BattleMaster::SelectRoundTurnOperator(bool is1P, bool useSub) {
        char& turnOp = is1P ? m_p1TurnOperator : m_p2TurnOperator;
        const char fixedOp = is1P ? m_p1DraftedOperator : m_p2DraftedOperator;
        const char subOp = is1P ? m_p1SubOperator : m_p2SubOperator;
        bool& usingSub = is1P ? m_p1UsingSub : m_p2UsingSub;

        if (useSub && subOp != '\0') {
            turnOp = subOp;
            usingSub = true;
        }
        else {
            turnOp = fixedOp;
            usingSub = false;
        }
        SyncRoundTurnOperator(is1P);
    }

    void BattleMaster::ConsumeRoundSubIfUsed(bool is1P) {
        bool& usingSub = is1P ? m_p1UsingSub : m_p2UsingSub;
        char& sub = is1P ? m_p1SubOperator : m_p2SubOperator;
        if (usingSub && sub != '\0') {
            AddLog(std::string("【サブ演算子使用】 ") + (is1P ? "1P" : "2P") +
                " [" + std::string(1, sub) + "] を消費");
            sub = '\0';
        }
        usingSub = false;
        SelectRoundTurnOperator(is1P, false);
    }

    bool BattleMaster::HandleRoundOperatorMouse(bool is1P, const Vector2& mousePos) {
        if (!m_rule.IsRoundBattle()) return false;

        constexpr int y = 850;
        constexpr int h = 70;
        constexpr int fixedX = 610;
        constexpr int fixedW = 330;
        constexpr int subX = 980;
        constexpr int subW = 330;

        if (CheckButtonClick(fixedX, y, fixedW, h, mousePos)) {
            SelectRoundTurnOperator(is1P, false);
            ProceduralAudio::GetInstance().PlayPowerSE(2);
            return true;
        }

        const char sub = is1P ? m_p1SubOperator : m_p2SubOperator;
        if (sub != '\0' && CheckButtonClick(subX, y, subW, h, mousePos)) {
            SelectRoundTurnOperator(is1P, true);
            ProceduralAudio::GetInstance().PlayPowerSE(2);
            return true;
        }
        return false;
    }

    int BattleMaster::GetRoundChipOperandForActor(const UnitBase& actor) const {
        const IntVector2 pos = actor.GetGridPos();
        return m_mapGrid.GetNumberChipAt(pos.x, pos.y);
    }

    std::vector<IntVector2> BattleMaster::BuildRoundChipPlacementCells(bool is1P) const {
        std::vector<IntVector2> result;
        const UnitBase* unit = GetUnitBySide(is1P);
        if (!unit) return result;

        const IntVector2 base = unit->GetGridPos();
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                const IntVector2 pos{ base.x + dx, base.y + dy };
                if (!m_mapGrid.IsWithinBounds(pos.x, pos.y)) continue;
                if (IsOccupiedByUnit(pos)) continue;
                if (m_mapGrid.HasAnyItemAt(pos.x, pos.y)) continue;
                result.push_back(pos);
            }
        }
        return result;
    }

    bool BattleMaster::ExecuteRoundCalculation(
        UnitBase& actor,
        int operand,
        bool operandWasChip,
        IntVector2 chipSource) {

        const bool is1P = (&actor == m_player.get());
        const char op = is1P ? m_p1TurnOperator : m_p2TurnOperator;
        const int left = actor.GetNumber();

        const BattleCalculationResult calc =
            m_rule.CalculateRoundArithmeticResult(left, operand, op);

        if (!calc.valid) {
            AddLog("【計算不可】 その演算は成立しません。");
            ProceduralAudio::GetInstance().PlayErrorSE();
            return false;
        }

        if (operandWasChip) {
            const int consumed = m_mapGrid.PickUpNumberChip(chipSource.x, chipSource.y);
            if (consumed == 0) return false;
        }

        actor.SetNumber(calc.normalizedValue);

        m_roundResultPending = true;
        m_roundChipPlacementPending = false;
        m_roundPendingIs1P = is1P;
        m_roundPendingResult = calc.normalizedValue;
        m_roundPendingRaw = calc.rawValue;
        m_roundPendingOperand = operand;
        m_roundPendingOperandWasChip = operandWasChip;
        m_roundPendingChipSource = chipSource;

        AddLog("【計算】 " + std::to_string(left) + " " +
            std::string(1, op) + " " + std::to_string(operand) +
            " = " + std::to_string(calc.rawValue) +
            " -> [" + std::to_string(calc.normalizedValue) + "]");

        if (op == '/') {
            const IntVector2 nodePos = m_rule.GetWarpGrid(left, operand);
            if (!actor.HasWarpNode(nodePos)) {
                actor.AddWarpNode(nodePos);
                AddLog("【ワープ】 (" + std::to_string(left) + "," +
                    std::to_string(operand) + ") にワープを生成");
            }
        }

        ProceduralAudio::GetInstance().PlayPowerSE(calc.normalizedValue);
        return true;
    }

    void BattleMaster::ResolveRoundResultToTotal() {
        if (!m_roundResultPending) return;

        Fraction& total = m_roundPendingIs1P ? m_p1RoundScore : m_p2RoundScore;
        const Fraction next = total + Fraction(m_roundPendingResult);
        if (next > Fraction(m_roundTarget)) {
            AddLog("【加算不可】 目標値を超えるため合計値へ加算できません。");
            ProceduralAudio::GetInstance().PlayErrorSE();
            return;
        }

        total = next;
        AddLog(std::string("【合計値】 ") +
            (m_roundPendingIs1P ? "1P " : "2P ") +
            "+ " + std::to_string(m_roundPendingResult) +
            " -> " + total.ToString());

        m_roundResultPending = false;
        m_roundChipPlacementPending = false;

        if (m_rule.IsRoundTargetReached(total, m_roundTarget)) {
            EndRound(m_roundPendingIs1P ? 1 : 2);
        }
        else {
            FinishActionPhase(m_roundPendingIs1P);
        }
    }

    bool BattleMaster::BeginRoundChipPlacement() {
        if (!m_roundResultPending) return false;
        const auto cells = BuildRoundChipPlacementCells(m_roundPendingIs1P);
        if (cells.empty()) {
            AddLog("【チップ配置不可】 配置できる周囲マスがありません。");
            ProceduralAudio::GetInstance().PlayErrorSE();

            const Fraction& total = m_roundPendingIs1P ? m_p1RoundScore : m_p2RoundScore;
            if (total + Fraction(m_roundPendingResult) > Fraction(m_roundTarget)) {
                const bool is1P = m_roundPendingIs1P;
                AddLog("【結果消滅】 合計値にも加えられないため、この計算結果は消滅します。");
                m_roundResultPending = false;
                m_roundChipPlacementPending = false;
                FinishActionPhase(is1P);
            }
            return false;
        }

        m_roundChipPlacementPending = true;
        AddLog("【数字チップ】 配置するマスをクリック");
        return true;
    }

    bool BattleMaster::PlacePendingRoundChip(IntVector2 pos) {
        if (!m_roundResultPending || !m_roundChipPlacementPending) return false;

        const auto cells = BuildRoundChipPlacementCells(m_roundPendingIs1P);
        if (std::find(cells.begin(), cells.end(), pos) == cells.end()) {
            ProceduralAudio::GetInstance().PlayErrorSE();
            return false;
        }

        m_mapGrid.SetNumberChipAt(pos.x, pos.y, m_roundPendingResult);
        AddLog("【チップ配置】 [" + std::to_string(m_roundPendingResult) + "] -> (" +
            std::to_string(pos.x + 1) + "," + std::to_string(9 - pos.y) + ")");

        const bool is1P = m_roundPendingIs1P;
        m_roundResultPending = false;
        m_roundChipPlacementPending = false;
        FinishActionPhase(is1P);
        return true;
    }

    bool BattleMaster::GetRoundActionClick(bool is1P, Vector2& mousePos) {
        bool isClick = false;
        const bool isOnline = (NetworkManager::GetInstance() != nullptr &&
            NetworkManager::GetInstance()->GetState() != NetworkManager::State::OFFLINE);

        bool isMyTurn = true;
        if (isOnline) {
            isMyTurn = (is1P == NetworkManager::GetInstance()->IsHost());
        }

        if (isOnline) {
            if (isMyTurn) {
                auto& input = InputManager::GetInstance();
                if (input.IsMouseLeftTrg()) {
                    isClick = true;
                    mousePos = input.GetMousePos();

                    BattlePacket packet;
                    packet.actionType = NetAction::ACTION;
                    packet.targetX = static_cast<int>(mousePos.x);
                    packet.targetY = static_cast<int>(mousePos.y);
                    NetworkManager::GetInstance()->SendBattlePacket(packet);
                }
            }
            else {
                BattlePacket packet;
                if (NetworkManager::GetInstance()->ReceiveBattlePacket(packet) &&
                    packet.actionType == NetAction::ACTION) {
                    isClick = true;
                    mousePos = Vector2(
                        static_cast<float>(packet.targetX),
                        static_cast<float>(packet.targetY));
                }
            }
        }
        else {
            auto& input = InputManager::GetInstance();
            if (input.IsMouseLeftTrg()) {
                isClick = true;
                mousePos = input.GetMousePos();
            }
        }
        return isClick;
    }

    bool BattleMaster::HandleRoundPendingActionInput(bool is1P) {
        if (!m_rule.IsRoundBattle() || (!m_roundResultPending && !m_roundChipPlacementPending)) {
            return false;
        }

        if (m_roundPendingIs1P != is1P) return true;

        Vector2 mousePos;
        if (!GetRoundActionClick(is1P, mousePos)) return true;

        if (m_roundChipPlacementPending) {
            const IntVector2 grid = m_mapGrid.ScreenToGrid(mousePos);
            PlacePendingRoundChip(grid);
            return true;
        }

        constexpr int y = 650;
        constexpr int h = 90;
        constexpr int totalX = 700;
        constexpr int chipX = 980;
        constexpr int w = 240;

        if (CheckButtonClick(totalX, y, w, h, mousePos)) {
            ResolveRoundResultToTotal();
        }
        else if (CheckButtonClick(chipX, y, w, h, mousePos)) {
            BeginRoundChipPlacement();
        }
        return true;
    }


    BattleViewData BattleMaster::BuildBattleViewData() const {
        BattleViewData view;
        view.map = &m_mapGrid;

        switch (m_currentPhase) {
        case Phase::P1_TurnStart: view.phase = BattleViewPhase::P1_TurnStart; break;
        case Phase::P1_Move:      view.phase = BattleViewPhase::P1_Move; break;
        case Phase::P1_Action:    view.phase = BattleViewPhase::P1_Action; break;
        case Phase::P2_TurnStart: view.phase = BattleViewPhase::P2_TurnStart; break;
        case Phase::P2_Move:      view.phase = BattleViewPhase::P2_Move; break;
        case Phase::P2_Action:    view.phase = BattleViewPhase::P2_Action; break;
        case Phase::FINISH:       view.phase = BattleViewPhase::FINISH; break;
        }

        if (m_rule.IsRoundBattle()) view.ruleMode = BattleViewRuleMode::ROUND_BATTLE;
        else if (m_rule.IsZeroOne()) view.ruleMode = BattleViewRuleMode::ZERO_ONE;
        else view.ruleMode = BattleViewRuleMode::CLASSIC;

        switch (m_roundPhase) {
        case RoundPhase::ROUND_START:       view.roundPhase = BattleRoundViewPhase::ROUND_START; break;
        case RoundPhase::SELECT_P1_NUMBER:  view.roundPhase = BattleRoundViewPhase::SELECT_P1_NUMBER; break;
        case RoundPhase::SELECT_P2_NUMBER:  view.roundPhase = BattleRoundViewPhase::SELECT_P2_NUMBER; break;
        case RoundPhase::TARGET_REVEAL:     view.roundPhase = BattleRoundViewPhase::TARGET_REVEAL; break;
        case RoundPhase::DRAFT_P1_OPERATOR: view.roundPhase = BattleRoundViewPhase::DRAFT_P1_OPERATOR; break;
        case RoundPhase::DRAFT_P2_OPERATOR: view.roundPhase = BattleRoundViewPhase::DRAFT_P2_OPERATOR; break;
        case RoundPhase::PLACE_OPERATORS:   view.roundPhase = BattleRoundViewPhase::PLACE_OPERATORS; break;
        case RoundPhase::BATTLE:            view.roundPhase = BattleRoundViewPhase::BATTLE; break;
        case RoundPhase::ROUND_END:         view.roundPhase = BattleRoundViewPhase::ROUND_END; break;
        case RoundPhase::INACTIVE:          view.roundPhase = BattleRoundViewPhase::INACTIVE; break;
        }
        view.roundSetupActive = m_rule.IsRoundBattle() && m_roundPhase != RoundPhase::INACTIVE && m_roundPhase != RoundPhase::BATTLE;
        view.roundNumber = m_roundNumber;
        view.p1RoundStartNumber = m_p1RoundStartNumber;
        view.p2RoundStartNumber = m_p2RoundStartNumber;
        view.roundTarget = m_roundTarget;
        view.roundNumberCursor = m_roundNumberCursor;
        view.roundOperators = m_roundOperators;
        view.roundOperatorAvailable = m_roundOperatorAvailable;
        view.roundOperatorCursor = m_roundOperatorCursor;
        view.p1DraftedOperator = m_p1DraftedOperator;
        view.p2DraftedOperator = m_p2DraftedOperator;
        view.roundPlacedOperator1 = m_roundPlacedOperator1;
        view.roundPlacedOperator2 = m_roundPlacedOperator2;
        view.roundPlacedPos1 = m_roundPlacedPos1;
        view.roundPlacedPos2 = m_roundPlacedPos2;
        view.roundWinner = m_roundWinner;
        view.p1RoundWins = m_p1RoundWins;
        view.p2RoundWins = m_p2RoundWins;
        view.p1FixedOperator = m_p1DraftedOperator;
        view.p2FixedOperator = m_p2DraftedOperator;
        view.p1SubOperator = m_p1SubOperator;
        view.p2SubOperator = m_p2SubOperator;
        view.p1TurnOperator = m_p1TurnOperator;
        view.p2TurnOperator = m_p2TurnOperator;
        view.p1UsingSub = m_p1UsingSub;
        view.p2UsingSub = m_p2UsingSub;
        view.roundResultPending = m_roundResultPending;
        view.roundChipPlacementPending = m_roundChipPlacementPending;
        view.roundPendingIs1P = m_roundPendingIs1P;
        view.roundPendingResult = m_roundPendingResult;
        view.roundPendingRaw = m_roundPendingRaw;
        if (m_roundResultPending) {
            const Fraction& total = m_roundPendingIs1P ? m_p1RoundScore : m_p2RoundScore;
            view.roundCanAddTotal =
                !(total + Fraction(m_roundPendingResult) > Fraction(m_roundTarget));
        }
        if (IsOnlineBattle()) {
            if (m_roundPhase == RoundPhase::SELECT_P1_NUMBER || m_roundPhase == RoundPhase::DRAFT_P1_OPERATOR) {
                view.roundWaitingForRemote = !IsLocalRoundController(true);
            }
            else if (m_roundPhase == RoundPhase::SELECT_P2_NUMBER || m_roundPhase == RoundPhase::DRAFT_P2_OPERATOR) {
                view.roundWaitingForRemote = !IsLocalRoundController(false);
            }
        }
        view.is1PTurn = Is1PTurn();
        view.gameOver = IsGameOver();
        view.playerSelected = m_isPlayerSelected;
        view.totalTurns = m_mapGrid.GetTotalTurns();
        view.targetScore = m_rule.IsRoundBattle() ? m_roundTarget : m_rule.GetTargetScore();
        view.turnStartTimer = m_turnStartTimer;
        view.finishTimer = m_finishTimer;
        view.hoverGrid = m_hoverGrid;

        auto fillUnit = [&](BattleUnitView& dst, UnitBase* unit, bool is1P) {
            if (!unit) return;
            dst.unit = unit;
            dst.isNPC = is1P ? m_is1P_NPC : m_is2P_NPC;
            dst.activeTurn = (is1P == view.is1PTurn) && m_currentPhase != Phase::FINISH;
            dst.number = unit->GetNumber();
            dst.stocks = unit->GetStocks();
            dst.maxStocks = unit->GetMaxStocks();
            dst.op = unit->GetOp();
            dst.gridPos = unit->GetGridPos();
            const Fraction& score = m_rule.IsRoundBattle()
                ? (is1P ? m_p1RoundScore : m_p2RoundScore)
                : (is1P ? m_p1ZeroOneScore : m_p2ZeroOneScore);
            dst.score = { score.n, score.d };
            dst.displayScore = is1P ? GetP1DisplayScore() : GetP2DisplayScore();
            dst.moveDistance = m_rule.GetMoveDistance(dst.number);
            dst.moveDirectionDots = m_rule.BuildBaseMoveDirectionDots(dst.number);
            dst.warpNodes = unit->GetWarpNodes();
            };

        fillUnit(view.p1, m_player.get(), true);
        fillUnit(view.p2, m_enemy.get(), false);

        UnitBase* activeActor = GetActiveUnit();
        UnitBase* activeTarget = GetTargetUnit();
        const bool activeIs1P = (activeActor && activeActor == m_player.get());

        if (activeActor) {
            view.hasActiveUnit = true;
            view.activeUnitIs1P = activeIs1P;
            view.activeGrid = activeActor->GetGridPos();
        }

        // 敵危険範囲は「相手が次に移動できる場所」をMaster側で確定して渡す。
        if (activeTarget && activeTarget->GetStocks() > 0 && !activeTarget->IsMoving()) {
            IntVector2 start = activeTarget->GetGridPos();
            for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
                for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                    IntVector2 target{ x, y };
                    if (target == start) continue;
                    int baseCost = 0;
                    int combinedCost = 0;
                    bool base = CanMove(activeTarget->GetNumber(), '\0', start, target, baseCost);
                    bool combined = CanMove(activeTarget->GetNumber(), activeTarget->GetOp(), start, target, combinedCost);
                    if (combined) {
                        view.enemyDangerCells.push_back({ target, combinedCost, base, activeTarget->HasWarpNode(target) });
                    }
                }
            }
        }

        bool isMovePhase = (m_currentPhase == Phase::P1_Move || m_currentPhase == Phase::P2_Move);
        bool isActionPhase = (m_currentPhase == Phase::P1_Action || m_currentPhase == Phase::P2_Action);

        int previewCost = 0;
        int previewNumber = activeActor ? activeActor->GetNumber() : 0;
        int previewStocks = activeActor ? activeActor->GetStocks() : 0;
        char previewOp = activeActor ? activeActor->GetOp() : '\0';
        bool movePreview = false;
        bool previewNextToEnemy = false;

        if (m_isPlayerSelected && activeActor && !activeActor->IsMoving() && isMovePhase) {
            IntVector2 start = activeActor->GetGridPos();
            for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
                for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                    IntVector2 target{ x, y };
                    if (target == start) continue;
                    int baseCost = 0;
                    int combinedCost = 0;
                    bool base = CanMove(activeActor->GetNumber(), '\0', start, target, baseCost);
                    bool combined = CanMove(activeActor->GetNumber(), activeActor->GetOp(), start, target, combinedCost);
                    if (combined || activeActor->HasWarpNode(target)) {
                        view.movableCells.push_back({ target, combinedCost, base, activeActor->HasWarpNode(target) });
                    }
                }
            }

            if (m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
                if (m_hoverGrid == start) {
                    movePreview = true;
                    previewCost = 1;
                }
                else if (CanMove(activeActor->GetNumber(), activeActor->GetOp(), start, m_hoverGrid, previewCost)) {
                    movePreview = true;
                }

                if (movePreview) {
                    view.hoverMoveValid = true;
                    view.hoverMoveGrid = m_hoverGrid;
                    view.hoverMoveCost = previewCost;

                    BattleSimulatedUnitState moved = m_rule.IsRoundBattle()
                        ? m_rule.SimulateRoundMoveCost(activeActor->GetNumber(), activeActor->GetStocks(), previewCost)
                        : m_rule.SimulateMoveCost(activeActor->GetNumber(), activeActor->GetStocks(), activeActor->GetMaxStocks(), previewCost);
                    previewNumber = moved.number;
                    previewStocks = moved.stocks;

                    BattleUnitView& activeView = activeIs1P ? view.p1 : view.p2;
                    activeView.hasPowerPreview = true;
                    activeView.previewNumber = previewNumber;
                    activeView.previewStocks = previewStocks;
                    activeView.previewDefeated = moved.defeated;
                    if (!moved.defeated && previewNumber >= 1 && previewNumber <= 9) {
                        activeView.previewMoveDistance = m_rule.GetMoveDistance(previewNumber);
                        activeView.previewMoveDirectionDots = m_rule.BuildBaseMoveDirectionDots(previewNumber);
                    }

                    char itemOnGrid = m_mapGrid.GetItemAt(m_hoverGrid.x, m_hoverGrid.y);
                    if (itemOnGrid != '\0') previewOp = itemOnGrid;

                    if (activeTarget) {
                        IntVector2 targetPos = activeTarget->GetGridPos();
                        previewNextToEnemy = (m_rule.IsAdjacent(m_hoverGrid, targetPos) && previewOp != '\0');
                    }
                }
            }
        }

        IntVector2 actorPos = activeActor ? activeActor->GetGridPos() : IntVector2{ -1, -1 };
        IntVector2 targetPos = activeTarget ? activeTarget->GetGridPos() : IntVector2{ -1, -1 };
        view.canAttack = activeActor && activeTarget && m_rule.IsAdjacent(actorPos, targetPos);
        view.hasOperator = activeActor && activeActor->GetOp() != '\0';
        view.humanActionPhase = isActionPhase && ((view.is1PTurn && !m_is1P_NPC) || (!view.is1PTurn && !m_is2P_NPC));

        Vector2 mousePos = InputManager::GetInstance().GetMousePos();
        if (m_rule.IsRoundBattle() && activeActor) {
            const bool activeIsHuman =
                (activeIs1P && !m_is1P_NPC) || (!activeIs1P && !m_is2P_NPC);
            if (activeIsHuman) {
                view.hoverRoundFixedButton =
                    CheckButtonClick(610, 850, 330, 70, mousePos);
                view.hoverRoundSubButton =
                    CheckButtonClick(980, 850, 330, 70, mousePos);
                view.hoverRoundPlayerCalcButton =
                    CheckButtonClick(610, 960, 210, 60, mousePos);
                view.hoverRoundChipCalcButton =
                    CheckButtonClick(855, 960, 210, 60, mousePos);
                view.hoverRoundTotalButton =
                    CheckButtonClick(700, 650, 240, 90, mousePos);
                view.hoverRoundChipButton =
                    CheckButtonClick(980, 650, 240, 90, mousePos);
            }

            view.roundPlayerCalcAvailable =
                activeTarget && m_rule.IsAdjacent(activeActor->GetGridPos(), activeTarget->GetGridPos());
            view.roundChipOperand =
                m_mapGrid.GetNumberChipAt(activeActor->GetGridPos().x, activeActor->GetGridPos().y);
            view.roundChipCalcAvailable = view.roundChipOperand > 0;

            if (m_roundChipPlacementPending) {
                view.roundChipPlacementCells =
                    BuildRoundChipPlacementCells(m_roundPendingIs1P);
            }
        }

        view.hoverNoActionButton = CheckButtonClick(1100, 960, 220, 60, mousePos);
        view.hoverEndTurnButton = CheckButtonClick(750, 960, 420, 60, mousePos);
        view.hoverPauseButton = CheckButtonClick(SCREEN_W - 180, 10, 160, 50, mousePos);

        bool hoverSelf = false;
        bool hoverEnemy = false;
        if (view.canAttack && view.hasOperator && activeActor && activeTarget) {
            if (CheckButtonClick(600, 960, 220, 60, mousePos)) hoverSelf = true;
            else if (CheckButtonClick(850, 960, 220, 60, mousePos)) hoverEnemy = true;
            else if (CheckButtonClick(40, 100, 500, 650, mousePos)) {
                if (view.is1PTurn) hoverSelf = true; else hoverEnemy = true;
            }
            else if (CheckButtonClick(1380, 100, 500, 650, mousePos)) {
                if (view.is1PTurn) hoverEnemy = true; else hoverSelf = true;
            }
            else if (m_hoverGrid == actorPos) hoverSelf = true;
            else if (m_hoverGrid == targetPos) hoverEnemy = true;
        }

        int calcLeft = 0;
        int calcRight = 0;
        char calcOp = '\0';

        if (!m_rule.IsRoundBattle() && isActionPhase && view.canAttack && view.hasOperator && activeActor && activeTarget) {
            view.calculation.visible = true;
            calcLeft = activeActor->GetNumber();
            calcRight = activeTarget->GetNumber();
            calcOp = activeActor->GetOp();
        }
        else if (!m_rule.IsRoundBattle() && isMovePhase && movePreview && previewNextToEnemy && activeActor && activeTarget && previewOp != '\0') {
            view.calculation.visible = true;
            view.calculation.movePreview = true;
            calcLeft = previewNumber;
            calcRight = activeTarget->GetNumber();
            calcOp = previewOp;
            view.calculation.willGetNewOperator = (activeActor->GetOp() != previewOp);
        }

        if (view.calculation.visible) {
            view.calculation.leftNumber = calcLeft;
            view.calculation.rightNumber = calcRight;
            view.calculation.op = calcOp;
            view.calculation.hoverSelf = hoverSelf;
            view.calculation.hoverEnemy = hoverEnemy;

            BattleCalculationResult calc = m_rule.CalculateBattleResult(calcLeft, calcRight, calcOp);
            view.calculation.intResult = calc.intValue;
            view.calculation.fractionResult = { calc.fraction.n, calc.fraction.d };
            view.calculation.cleanDivide = calc.cleanDivide;

            if (calcOp == '/' && calcRight != 0) {
                view.calculation.createsWarp = true;
                view.calculation.warpGrid = m_rule.GetWarpGrid(calcLeft, calcRight);
                view.calculation.warpDisplay = { calcLeft, calcRight };
            }

            int selfNumber = activeActor->GetNumber();
            int selfStocks = activeActor->GetStocks();
            if (view.calculation.movePreview) {
                selfNumber = previewNumber;
                selfStocks = previewStocks;
            }

            Fraction selfScore = m_rule.IsRoundBattle()
                ? (activeIs1P ? m_p1RoundScore : m_p2RoundScore)
                : (activeIs1P ? m_p1ZeroOneScore : m_p2ZeroOneScore);
            Fraction enemyScore = m_rule.IsRoundBattle()
                ? (activeIs1P ? m_p2RoundScore : m_p1RoundScore)
                : (activeIs1P ? m_p2ZeroOneScore : m_p1ZeroOneScore);

            BattleSimulatedUnitState selfState;
            BattleSimulatedUnitState enemyState;
            if (m_rule.IsClassic()) {
                selfState = m_rule.SimulateClassicResult(selfNumber, selfStocks, activeActor->GetMaxStocks(), calc.intValue, calcOp, calc.cleanDivide);
                enemyState = m_rule.SimulateClassicResult(activeTarget->GetNumber(), activeTarget->GetStocks(), activeTarget->GetMaxStocks(), calc.intValue, calcOp, calc.cleanDivide);
            }
            else if (m_rule.IsRoundBattle()) {
                selfState = m_rule.SimulateRoundBattleResult(selfNumber, selfStocks, selfScore, m_roundTarget, calc.intValue, calc.fraction, calcOp, calc.cleanDivide);
                enemyState = m_rule.SimulateRoundBattleResult(activeTarget->GetNumber(), activeTarget->GetStocks(), enemyScore, m_roundTarget, calc.intValue, calc.fraction, calcOp, calc.cleanDivide);
            }
            else {
                selfState = m_rule.SimulateZeroOneResult(selfNumber, selfStocks, selfScore, calc.intValue, calc.fraction, calcOp, calc.cleanDivide);
                enemyState = m_rule.SimulateZeroOneResult(activeTarget->GetNumber(), activeTarget->GetStocks(), enemyScore, calc.intValue, calc.fraction, calcOp, calc.cleanDivide);
            }

            auto toView = [](const BattleSimulatedUnitState& src) {
                BattleTargetResultView dst;
                dst.valid = src.valid;
                dst.defeated = src.defeated;
                dst.number = src.number;
                dst.stocks = src.stocks;
                dst.stockDelta = src.stockDelta;
                dst.hasScore = src.hasScore;
                dst.score = { src.score.n, src.score.d };
                return dst;
                };
            view.calculation.selfResult = toView(selfState);
            view.calculation.enemyResult = toView(enemyState);

            // 行動フェーズで反映先にカーソルを置いた場合、左右カードにも同じ予測値を表示する。
            if (!view.calculation.movePreview && (hoverSelf || hoverEnemy)) {
                bool targetIs1P = hoverSelf ? activeIs1P : !activeIs1P;
                const BattleSimulatedUnitState& selected = hoverSelf ? selfState : enemyState;
                BattleUnitView& unitView = targetIs1P ? view.p1 : view.p2;

                if (selected.valid) {
                    unitView.hasPowerPreview = true;
                    unitView.previewNumber = selected.number;
                    unitView.previewStocks = selected.stocks;
                    unitView.previewDefeated = selected.defeated;
                    if (!selected.defeated && selected.number >= 1 && selected.number <= 9) {
                        unitView.previewMoveDistance = m_rule.GetMoveDistance(selected.number);
                        unitView.previewMoveDirectionDots = m_rule.BuildBaseMoveDirectionDots(selected.number);
                    }
                    if (selected.hasScore) {
                        unitView.hasScorePreview = true;
                        unitView.previewScore = { selected.score.n, selected.score.d };
                    }
                }
            }
        }

        view.ruleLines = m_rule.GetRuleLines();

        return view;
    }


    void BattleMaster::HandleMoveInput(UnitBase& activeUnit, Phase nextPhase) {
        if (activeUnit.IsMoving()) return;

        bool isClick = false;
        Vector2 mousePos;

        // ★ 判定条件をOFFLINE以外で統一
        bool isOnline = (NetworkManager::GetInstance() != nullptr && NetworkManager::GetInstance()->GetState() != NetworkManager::State::OFFLINE);
        bool isMyTurn = true;
        if (isOnline) {
            bool isHost = NetworkManager::GetInstance()->IsHost();
            isMyTurn = (Is1PTurn() == isHost);
        }

        if (isOnline) {
            if (isMyTurn) {
                auto& input = InputManager::GetInstance();
                if (input.IsMouseLeftTrg()) {
                    isClick = true;
                    mousePos = input.GetMousePos();

                    BattlePacket packet;
                    packet.actionType = NetAction::MOVE;
                    packet.targetX = (int)mousePos.x;
                    packet.targetY = (int)mousePos.y;
                    NetworkManager::GetInstance()->SendBattlePacket(packet);
                }
            }
            else {
                BattlePacket packet;
                if (NetworkManager::GetInstance()->ReceiveBattlePacket(packet)) {
                    isClick = true;
                    mousePos = Vector2((float)packet.targetX, (float)packet.targetY);
                }
            }
        }
        else {
            auto& input = InputManager::GetInstance();
            if (input.IsMouseLeftTrg()) {
                isClick = true;
                mousePos = input.GetMousePos();
            }
        }

        if (!isClick) return;

        if (m_rule.IsRoundBattle() && HandleRoundOperatorMouse(Is1PTurn(), mousePos)) {
            return;
        }

        IntVector2 targetGrid = m_mapGrid.ScreenToGrid(mousePos);
        IntVector2 pos = activeUnit.GetGridPos();

        if (!m_isPlayerSelected) {
            if (targetGrid == pos) {
                m_isPlayerSelected = true;
                ProceduralAudio::GetInstance().PlayPowerSE(3);
            }
            return;
        }

        if (targetGrid == pos) {
            AddPowerWithBattery(activeUnit, -1, "待機");
            m_isPlayerSelected = false;
            m_currentPhase = nextPhase;
            ProceduralAudio::GetInstance().PlayPowerSE(1);
            return;
        }

        int cost = 0;
        if (!m_mapGrid.IsWithinBounds(targetGrid.x, targetGrid.y) ||
            !CanMove(activeUnit.GetNumber(), activeUnit.GetOp(), pos, targetGrid, cost)) {
            m_isPlayerSelected = false;
            ProceduralAudio::GetInstance().PlayErrorSE();
            return;
        }

        ProceduralAudio::GetInstance().PlayPowerSE(5);
        AddPowerWithBattery(activeUnit, -cost, "移動");
        std::queue<Vector2> autoPath;
        int dx = std::abs(targetGrid.x - pos.x);
        int dy = std::abs(targetGrid.y - pos.y);
        if (Is1PTurn()) m_p1TotalMoves += std::max(dx, dy);
        else m_p2TotalMoves += std::max(dx, dy);

        std::string myName = Is1PTurn() ? "1P" : "2P";

        if (activeUnit.HasWarpNode(targetGrid) || (dx != dy && dx != 0 && dy != 0)) {
            autoPath.push(m_mapGrid.GetCellCenter(targetGrid.x, targetGrid.y));
            AddLog("【跳躍】 " + myName + " が (" + std::to_string(pos.x + 1) + "," + std::to_string(9 - pos.y) + ") から (" + std::to_string(targetGrid.x + 1) + "," + std::to_string(9 - targetGrid.y) + ") へワープ！ (パワー -1)");
            ProceduralAudio::GetInstance().PlayPowerSE(8);
        }
        else {
            int stepX = (targetGrid.x > pos.x) ? 1 : (targetGrid.x < pos.x) ? -1 : 0;
            int stepY = (targetGrid.y > pos.y) ? 1 : (targetGrid.y < pos.y) ? -1 : 0;
            IntVector2 curr = pos;
            int maxSteps = std::max(dx, dy);
            for (int i = 0; i < maxSteps; ++i) {
                curr.x += stepX;
                curr.y += stepY;
                autoPath.push(m_mapGrid.GetCellCenter(curr.x, curr.y));
            }

            AddLog("【移動】 " + myName + " が (" + std::to_string(targetGrid.x + 1) + "," + std::to_string(9 - targetGrid.y) + ") へ移動(パワー -" + std::to_string(cost) + ")");
        }

        activeUnit.StartMove(targetGrid, autoPath);
        m_isPlayerSelected = false;
        m_currentPhase = nextPhase;
    }

    void BattleMaster::HandleActionInput(UnitBase& actor, UnitBase& targetUnit) {
        if (actor.IsMoving()) return;

        const bool is1P = (&actor == m_player.get());

        if (m_rule.IsRoundBattle()) {
            if (HandleRoundPendingActionInput(is1P)) return;

            IntVector2 pos = actor.GetGridPos();

            // SUBを移動で使用した場合、着地した時点で旧SUBを消費。
            // このあと新しいSUBを拾っても、新SUBまで消えないようにする。
            bool& usingSub = is1P ? m_p1UsingSub : m_p2UsingSub;
            char& sub = is1P ? m_p1SubOperator : m_p2SubOperator;
            char& turnOp = is1P ? m_p1TurnOperator : m_p2TurnOperator;
            if (usingSub) {
                const char usedOp = turnOp;
                AddLog(std::string("【サブ演算子使用】 ") + (is1P ? "1P" : "2P") +
                    " [" + std::string(1, usedOp) + "] を消費");
                sub = '\0';
                usingSub = false;
                turnOp = usedOp; // このターンの計算までは同じ演算子を使う
                actor.SetOp(usedOp);
            }

            const char pickedItem = m_mapGrid.PickUpItem(pos.x, pos.y);
            if (pickedItem != '\0') {
                sub = pickedItem;
                AddLog(std::string("【サブ演算子取得】 ") + (is1P ? "1P" : "2P") +
                    " [" + std::string(1, pickedItem) + "] をサブ演算子枠へ取得");
                ProceduralAudio::GetInstance().PlayPowerSE(
                    pickedItem == '+' ? 5 : pickedItem == '-' ? 2 : pickedItem == '*' ? 7 : 9);
            }

            const bool playerCalcAvailable =
                m_rule.IsAdjacent(pos, targetUnit.GetGridPos());
            const int chipOperand = GetRoundChipOperandForActor(actor);
            const bool chipCalcAvailable = chipOperand > 0;

            if (!playerCalcAvailable && !chipCalcAvailable) {
                AddLog("【待機】 計算対象がないため行動終了");
                FinishActionPhase(is1P);
                return;
            }

            Vector2 mousePos;
            if (!GetRoundActionClick(is1P, mousePos)) return;

            if (playerCalcAvailable &&
                CheckButtonClick(610, 960, 210, 60, mousePos)) {
                ExecuteRoundCalculation(
                    actor,
                    targetUnit.GetNumber(),
                    false,
                    IntVector2{ -1, -1 });
                return;
            }

            if (chipCalcAvailable &&
                CheckButtonClick(855, 960, 210, 60, mousePos)) {
                ExecuteRoundCalculation(actor, chipOperand, true, pos);
                return;
            }

            if (CheckButtonClick(1100, 960, 220, 60, mousePos)) {
                AddLog("【待機】 行動を終了");
                ProceduralAudio::GetInstance().PlayPowerSE(2);
                FinishActionPhase(is1P);
            }
            return;
        }

        IntVector2 pos = actor.GetGridPos();
        char pickedItem = m_mapGrid.PickUpItem(pos.x, pos.y);
        if (pickedItem != '\0') {
            actor.SetOp(pickedItem);
            std::string name = is1P ? "1P" : "2P";
            AddLog("【取得】 " + name + "が [" + std::string(1, pickedItem) + "] を取得！");

            if (is1P) m_p1OpCostPending = false;
            else m_p2OpCostPending = false;

            if (pickedItem == '+') ProceduralAudio::GetInstance().PlayPowerSE(5);
            else if (pickedItem == '-') ProceduralAudio::GetInstance().PlayPowerSE(2);
            else if (pickedItem == '*') ProceduralAudio::GetInstance().PlayPowerSE(7);
            else if (pickedItem == '/') ProceduralAudio::GetInstance().PlayPowerSE(9);
        }

        IntVector2 targetPos = targetUnit.GetGridPos();
        bool canAttack = m_rule.IsAdjacent(pos, targetPos);
        bool hasOp = (actor.GetOp() != '\0');

        if (!canAttack || !hasOp) {
            AddLog("【待機】 行動を終了");
            FinishActionPhase(is1P);
            return;
        }

        bool isClick = false;
        Vector2 mousePos;

        bool isOnline = (NetworkManager::GetInstance() != nullptr &&
            NetworkManager::GetInstance()->GetState() != NetworkManager::State::OFFLINE);
        bool isMyTurn = true;
        if (isOnline) {
            bool isHost = NetworkManager::GetInstance()->IsHost();
            isMyTurn = (is1P == isHost);
        }

        if (isOnline) {
            if (isMyTurn) {
                auto& input = InputManager::GetInstance();
                if (input.IsMouseLeftTrg()) {
                    isClick = true;
                    mousePos = input.GetMousePos();

                    BattlePacket packet;
                    packet.actionType = NetAction::ACTION;
                    packet.targetX = (int)mousePos.x;
                    packet.targetY = (int)mousePos.y;
                    NetworkManager::GetInstance()->SendBattlePacket(packet);
                }
            }
            else {
                BattlePacket packet;
                if (NetworkManager::GetInstance()->ReceiveBattlePacket(packet)) {
                    isClick = true;
                    mousePos = Vector2((float)packet.targetX, (float)packet.targetY);
                }
            }
        }
        else {
            auto& input = InputManager::GetInstance();
            if (input.IsMouseLeftTrg()) {
                isClick = true;
                mousePos = input.GetMousePos();
            }
        }

        if (!isClick) return;

        IntVector2 hoverGrid = m_mapGrid.ScreenToGrid(mousePos);
        bool actionDone = false;

        if (CheckButtonClick(600, 960, 220, 60, mousePos) ||
            hoverGrid == pos ||
            (CheckButtonClick(40, 100, 500, 650, mousePos) && is1P) ||
            (CheckButtonClick(1380, 100, 500, 650, mousePos) && !is1P)) {
            ProceduralAudio::GetInstance().PlayPowerSE(6);
            ExecuteBattle(actor, targetUnit, actor);
            actionDone = true;
        }
        else if (CheckButtonClick(850, 960, 220, 60, mousePos) ||
            hoverGrid == targetPos ||
            (CheckButtonClick(40, 100, 500, 650, mousePos) && !is1P) ||
            (CheckButtonClick(1380, 100, 500, 650, mousePos) && is1P)) {
            ProceduralAudio::GetInstance().PlayPowerSE(6);
            ExecuteBattle(actor, targetUnit, targetUnit);
            actionDone = true;
        }
        else if (CheckButtonClick(1100, 960, 220, 60, mousePos)) {
            ProceduralAudio::GetInstance().PlayPowerSE(2);
            AddLog("【待機】 行動を終了");
            actionDone = true;
        }

        if (actionDone) FinishActionPhase(is1P);
    }




    void BattleMaster::ExecuteAIAction(UnitBase* me, UnitBase* opp, bool is1P) {
        if (!me || !opp || me->IsMoving() || !m_ai) return;

        if (m_rule.IsRoundBattle()) {
            if (m_roundResultPending || m_roundChipPlacementPending) return;

            const std::string myName = is1P ? "1P" : "2P";
            const IntVector2 myPos = me->GetGridPos();

            bool& usingSub = is1P ? m_p1UsingSub : m_p2UsingSub;
            char& sub = is1P ? m_p1SubOperator : m_p2SubOperator;
            char& turnOp = is1P ? m_p1TurnOperator : m_p2TurnOperator;

            if (usingSub) {
                const char usedOp = turnOp;
                sub = '\0';
                usingSub = false;
                turnOp = usedOp;
                me->SetOp(usedOp);
                AddLog("【サブ演算子使用】 " + myName + " [" + std::string(1, usedOp) + "] を消費");
            }

            const char pickedItem = m_mapGrid.PickUpItem(myPos.x, myPos.y);
            if (pickedItem != '\0') {
                sub = pickedItem;
                AddLog("【サブ演算子取得】 " + myName + " [" + std::string(1, pickedItem) + "] を取得");
            }

            struct Candidate {
                int operand = 0;
                bool chip = false;
                IntVector2 chipPos{ -1, -1 };
                int eval = -9999999;
            };

            Candidate best;
            const Fraction& total = is1P ? m_p1RoundScore : m_p2RoundScore;
            const int remaining = m_roundTarget - static_cast<int>(total.n / total.d);

            auto evaluate = [&](int operand, bool chip, IntVector2 chipPos) {
                const auto calc = m_rule.CalculateRoundArithmeticResult(me->GetNumber(), operand, turnOp);
                if (!calc.valid) return;
                int eval = calc.normalizedValue * 100;
                if (calc.normalizedValue == remaining) eval += 1000000;
                else if (calc.normalizedValue > remaining) eval -= 3000;
                if (chip) eval += 80;
                if (eval > best.eval) {
                    best = Candidate{ operand, chip, chipPos, eval };
                }
                };

            if (m_rule.IsAdjacent(myPos, opp->GetGridPos())) {
                evaluate(opp->GetNumber(), false, IntVector2{ -1, -1 });
            }

            const int chipValue = m_mapGrid.GetNumberChipAt(myPos.x, myPos.y);
            if (chipValue > 0) {
                evaluate(chipValue, true, myPos);
            }

            if (best.operand <= 0) {
                AddLog("【待機】 " + myName + " は計算対象なし");
                FinishActionPhase(is1P);
            }
            else if (ExecuteRoundCalculation(*me, best.operand, best.chip, best.chipPos)) {
                const Fraction& nowTotal = is1P ? m_p1RoundScore : m_p2RoundScore;
                const Fraction next = nowTotal + Fraction(m_roundPendingResult);

                if (!(next > Fraction(m_roundTarget))) {
                    ResolveRoundResultToTotal();
                }
                else if (BeginRoundChipPlacement()) {
                    auto cells = BuildRoundChipPlacementCells(is1P);
                    if (!cells.empty()) {
                        // 盤面中央に近いマスを優先。再現可能な決定でテストしやすくする。
                        std::sort(cells.begin(), cells.end(), [](const IntVector2& a, const IntVector2& b) {
                            const int da = std::abs(a.x - 4) + std::abs(a.y - 4);
                            const int db = std::abs(b.x - 4) + std::abs(b.y - 4);
                            if (da != db) return da < db;
                            if (a.y != b.y) return a.y < b.y;
                            return a.x < b.x;
                            });
                        PlacePendingRoundChip(cells.front());
                    }
                }
                else {
                    FinishActionPhase(is1P);
                }
            }

            if (is1P) m_playerAIStarted = false;
            else m_enemyAIStarted = false;
            return;
        }

        const std::string myName = is1P ? "1P" : "2P";
        const IntVector2 myPos = me->GetGridPos();
        const char pickedItem = m_mapGrid.PickUpItem(myPos.x, myPos.y);

        if (pickedItem != '\0') {
            me->SetOp(pickedItem);
            AddLog("【取得】 " + myName + " が [" + std::string(1, pickedItem) + "] を取得");

            if (is1P) m_p1OpCostPending = false;
            else m_p2OpCostPending = false;

            if (pickedItem == '+') ProceduralAudio::GetInstance().PlayPowerSE(5);
            else if (pickedItem == '-') ProceduralAudio::GetInstance().PlayPowerSE(2);
            else if (pickedItem == '*') ProceduralAudio::GetInstance().PlayPowerSE(7);
            else if (pickedItem == '/') ProceduralAudio::GetInstance().PlayPowerSE(9);
        }

        BattleAI::ActionContext context;
        context.me = me;
        context.enemy = opp;
        context.is1P = is1P;
        context.rule = &m_rule;
        context.p1Score = m_p1ZeroOneScore;
        context.p2Score = m_p2ZeroOneScore;
        context.roundTarget = m_roundTarget;

        const BattleAI::ActionTarget target = m_ai->ChooseActionTarget(context);

        if (target == BattleAI::ActionTarget::SELF) {
            ProceduralAudio::GetInstance().PlayPowerSE(6);
            ExecuteBattle(*me, *opp, *me);
        }
        else if (target == BattleAI::ActionTarget::OPPONENT) {
            ProceduralAudio::GetInstance().PlayPowerSE(6);
            ExecuteBattle(*me, *opp, *opp);
        }
        else {
            AddLog("【待機】 " + myName + " は行動を完了");
            ProceduralAudio::GetInstance().PlayPowerSE(2);
        }

        FinishActionPhase(is1P);

        if (is1P) m_playerAIStarted = false;
        else m_enemyAIStarted = false;
    }

    void BattleMaster::Update() {

        if (NetworkManager::GetInstance()) {
            NetworkManager::GetInstance()->Update();
        }

        if (m_currentPhase == Phase::FINISH) {
            m_finishTimer++;
            if (m_finishTimer > 60) {
                auto& input = InputManager::GetInstance();
                if (input.IsMouseLeftTrg() || input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN)) {
                    int finalTimeMs = GetNowCount() - m_startTime;
                    int turns = m_mapGrid.GetTotalTurns();

                    BattleStats p1Stats = {
                        turns, finalTimeMs, m_p1TotalMoves, m_p1TotalOps, m_p1MaxDamage,
                        m_player ? m_player->GetMaxStocks() : 0
                    };
                    BattleStats p2Stats = {
                        turns, finalTimeMs, m_p2TotalMoves, m_p2TotalOps, m_p2MaxDamage,
                        m_enemy ? m_enemy->GetMaxStocks() : 0
                    };

                    const int winner = m_rule.IsP1Winner(
                        m_p1ZeroOneScore,
                        m_p2ZeroOneScore,
                        m_is1PWinner) ? 1 : 2;

                    auto* sm = SceneManager::GetInstance();
                    sm->SetBattleResult(winner, p1Stats, p2Stats);
                    sm->ChangeScene(SceneManager::SCENE_ID::RESULT);
                }
            }
            return;
        }

        if (m_rule.IsZeroOne()) {
            if (m_player && m_player->GetStocks() < m_player->GetMaxStocks()) m_player->AddStocks(m_player->GetMaxStocks() - m_player->GetStocks());
            if (m_enemy && m_enemy->GetStocks() < m_enemy->GetMaxStocks()) m_enemy->AddStocks(m_enemy->GetMaxStocks() - m_enemy->GetStocks());
        }

        auto& input = InputManager::GetInstance();
        if (m_player) m_player->Update();
        if (m_enemy)  m_enemy->Update();
        m_hoverGrid = m_mapGrid.ScreenToGrid(input.GetMousePos());

        int wheel = GetMouseWheelRotVol();
        if (wheel != 0 && m_ui) {
            Vector2 mousePos = input.GetMousePos();
            m_ui->ScrollLog(wheel, mousePos.x, mousePos.y);
        }

        if (m_ui) {
            int p1Num = m_player ? m_player->GetNumber() : 1;
            int p2Num = m_enemy ? m_enemy->GetNumber() : 1;
            m_ui->Update(m_effectIntensity, p1Num, p2Num);
        }

        // ラウンドバトルのセットアップ中は専用進行だけを更新し、
        // 旧Classic/Countのターン処理へは入れない。
        if (m_rule.IsRoundBattle() && m_roundPhase != RoundPhase::BATTLE) {
            UpdateRoundSetup();
            return;
        }

        if (m_effectIntensity > 0.0f) m_effectIntensity -= 0.05f;

        if (m_rule.IsZeroOne() || m_rule.IsRoundBattle()) {
            const Fraction& score1P = m_rule.IsRoundBattle() ? m_p1RoundScore : m_p1ZeroOneScore;
            const Fraction& score2P = m_rule.IsRoundBattle() ? m_p2RoundScore : m_p2ZeroOneScore;
            const float target1P = static_cast<float>(score1P.n) / static_cast<float>(score1P.d);
            const float target2P = static_cast<float>(score2P.n) / static_cast<float>(score2P.d);

            m_p1DisplayScore += (target1P - m_p1DisplayScore) * 0.15f;
            m_p2DisplayScore += (target2P - m_p2DisplayScore) * 0.15f;

            if (std::abs(target1P - m_p1DisplayScore) < 0.01f) m_p1DisplayScore = target1P;
            if (std::abs(target2P - m_p2DisplayScore) < 0.01f) m_p2DisplayScore = target2P;
        }

        switch (m_currentPhase) {
        case Phase::P1_TurnStart:
        case Phase::P2_TurnStart:
            m_turnStartTimer--;
            if (m_turnStartTimer <= 0) {
                m_currentPhase = (m_currentPhase == Phase::P1_TurnStart) ? Phase::P1_Move : Phase::P2_Move;
            }
            break;

        case Phase::P1_Move:
            if (m_player) ReserveOperatorUpkeepIfNeeded(*m_player, true);

            if (m_is1P_NPC) {
                if (m_aiWaitTimer > 0) m_aiWaitTimer--;
                else {
                    if (!m_playerAIStarted) { ExecuteAI(m_player.get(), m_enemy.get(), true); m_playerAIStarted = true; }
                    else if (m_player && !m_player->IsMoving()) {
                        m_currentPhase = Phase::P1_Action;
                        m_aiWaitTimer = 30;
                    }
                }
            }
            else HandleMoveInput(*m_player, Phase::P1_Action);
            break;

        case Phase::P1_Action:
            if (m_is1P_NPC) {
                if (m_aiWaitTimer > 0) m_aiWaitTimer--;
                else ExecuteAIAction(m_player.get(), m_enemy.get(), true);
            }
            else HandleActionInput(*m_player, *m_enemy);
            break;

        case Phase::P2_Move:
            if (m_enemy) ReserveOperatorUpkeepIfNeeded(*m_enemy, false);

            if (m_is2P_NPC) {
                if (m_aiWaitTimer > 0) m_aiWaitTimer--;
                else {
                    if (!m_enemyAIStarted) { ExecuteAI(m_enemy.get(), m_player.get(), false); m_enemyAIStarted = true; }
                    else if (m_enemy && !m_enemy->IsMoving()) {
                        m_currentPhase = Phase::P2_Action;
                        m_aiWaitTimer = 30;
                    }
                }
            }
            else HandleMoveInput(*m_enemy, Phase::P2_Action);
            break;

        case Phase::P2_Action:
            if (m_is2P_NPC) {
                if (m_aiWaitTimer > 0) m_aiWaitTimer--;
                else ExecuteAIAction(m_enemy.get(), m_player.get(), false);
            }
            else HandleActionInput(*m_enemy, *m_player);
            break;

        case Phase::FINISH:
            // FINISH は Update 冒頭で処理して return しているため、通常ここには到達しない。
            break;
        }

        int pauseBtnW = 160;
        int pauseBtnH = 50;
        int pauseBtnX = SCREEN_W - pauseBtnW - 20;
        int pauseBtnY = 10;

        if (CheckButtonClick(pauseBtnX, pauseBtnY, pauseBtnW, pauseBtnH, input.GetMousePos())) {
            static Vector2 prevM = input.GetMousePos();
            if (prevM.x != input.GetMousePos().x || prevM.y != input.GetMousePos().y) {
                ProceduralAudio::GetInstance().PlayPowerSE(2);
            }
            prevM = input.GetMousePos();

            if (input.IsMouseLeftTrg()) {
                SceneManager::GetInstance()->TogglePause();
                return;
            }
        }

        bool isDisplayCaughtUp = true;
        if (m_rule.IsZeroOne() || m_rule.IsRoundBattle()) {
            const Fraction& score1P = m_rule.IsRoundBattle() ? m_p1RoundScore : m_p1ZeroOneScore;
            const Fraction& score2P = m_rule.IsRoundBattle() ? m_p2RoundScore : m_p2ZeroOneScore;
            const float target1P = static_cast<float>(score1P.n) / static_cast<float>(score1P.d);
            const float target2P = static_cast<float>(score2P.n) / static_cast<float>(score2P.d);
            isDisplayCaughtUp = std::abs(m_p1DisplayScore - target1P) < 0.01f &&
                std::abs(m_p2DisplayScore - target2P) < 0.01f;
        }

        if (IsGameOver() && isDisplayCaughtUp && m_currentPhase != Phase::FINISH) {
            m_currentPhase = Phase::FINISH;
            m_finishTimer = 0;
            m_effectIntensity = 1.0f;
            ProceduralAudio::GetInstance().PlayPowerSE(9);
            ProceduralAudio::GetInstance().PlayErrorSE();
        }
    }








    void BattleMaster::PerformAIMove(UnitBase* me, IntVector2 bestTarget, int selectedCost, bool is1P) {
        if (!me) return;

        const IntVector2 myPos = me->GetGridPos();
        std::queue<Vector2> screenPath;
        const std::string myName = is1P ? "1P" : "2P";

        const bool isStay = (bestTarget == myPos);
        int actualCost = isStay ? 1 : selectedCost;
        if (me->HasWarpNode(bestTarget)) actualCost = 1;

        if (me->HasWarpNode(bestTarget)) {
            AddLog("【跳躍】 " + myName + " がワープを起動し (" +
                std::to_string(bestTarget.x + 1) + "," + std::to_string(9 - bestTarget.y) + ") に移動！");
            ProceduralAudio::GetInstance().PlayPowerSE(8);
        }
        else if (isStay) {
            AddLog("【待機】 " + myName + " はその場で動かずパワーを消費しました。");
            ProceduralAudio::GetInstance().PlayPowerSE(1);
        }
        else {
            AddLog("【移動】 " + myName + " が (" +
                std::to_string(bestTarget.x + 1) + "," + std::to_string(9 - bestTarget.y) + ") へ移動");
            ProceduralAudio::GetInstance().PlayPowerSE(5);
        }

        const int dx = std::abs(bestTarget.x - myPos.x);
        const int dy = std::abs(bestTarget.y - myPos.y);
        if (is1P) m_p1TotalMoves += std::max(dx, dy);
        else m_p2TotalMoves += std::max(dx, dy);

        if (me->HasWarpNode(bestTarget) || isStay || (dx != dy && dx != 0 && dy != 0)) {
            screenPath.push(m_mapGrid.GetCellCenter(bestTarget.x, bestTarget.y));
        }
        else {
            const int stepX = (bestTarget.x > myPos.x) ? 1 : (bestTarget.x < myPos.x) ? -1 : 0;
            const int stepY = (bestTarget.y > myPos.y) ? 1 : (bestTarget.y < myPos.y) ? -1 : 0;
            IntVector2 current = myPos;
            const int maxSteps = std::max(dx, dy);

            for (int i = 0; i < maxSteps; ++i) {
                current.x += stepX;
                current.y += stepY;
                screenPath.push(m_mapGrid.GetCellCenter(current.x, current.y));
            }
        }

        AddPowerWithBattery(*me, -actualCost, "移動");
        me->StartMove(bestTarget, screenPath);

        if (is1P) m_playerAIStarted = true;
        else m_enemyAIStarted = true;
    }

    void BattleMaster::ExecuteAI(UnitBase* me, UnitBase* opp, bool is1P) {
        if (!me || !opp || !m_ai) return;

        if (m_rule.IsRoundBattle()) {
            const char fixedOp = is1P ? m_p1DraftedOperator : m_p2DraftedOperator;
            const char subOp = is1P ? m_p1SubOperator : m_p2SubOperator;
            bool chooseSub = false;

            if (subOp != '\0') {
                const Fraction& total = is1P ? m_p1RoundScore : m_p2RoundScore;
                const int remaining = m_roundTarget - static_cast<int>(total.n / total.d);
                const auto fixedCalc = m_rule.CalculateRoundArithmeticResult(
                    me->GetNumber(), opp->GetNumber(), fixedOp);
                const auto subCalc = m_rule.CalculateRoundArithmeticResult(
                    me->GetNumber(), opp->GetNumber(), subOp);

                if (subCalc.valid && subCalc.normalizedValue == remaining) {
                    chooseSub = true;
                }
                else if (subCalc.valid &&
                    (!fixedCalc.valid ||
                        (fixedCalc.normalizedValue > remaining && subCalc.normalizedValue <= remaining))) {
                    chooseSub = true;
                }
            }
            SelectRoundTurnOperator(is1P, chooseSub);
        }

        BattleAI::MoveContext context;
        context.map = &m_mapGrid;
        context.me = me;
        context.enemy = opp;
        context.is1P = is1P;
        context.rule = &m_rule;
        context.p1Score = m_rule.IsRoundBattle() ? m_p1RoundScore : m_p1ZeroOneScore;
        context.p2Score = m_rule.IsRoundBattle() ? m_p2RoundScore : m_p2ZeroOneScore;
        context.roundTarget = m_roundTarget;

        const IntVector2 myPos = me->GetGridPos();
        const int currentNumber = me->GetNumber();
        const char currentOp = me->GetOp();

        for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                const IntVector2 target{ x, y };
                int cost = 1;

                bool canGo = (target == myPos);
                if (!canGo) {
                    canGo = CanMove(currentNumber, currentOp, myPos, target, cost);
                }

                if (canGo) {
                    context.candidates.push_back({ target, cost });
                }
            }
        }

        const BattleAI::MoveDecision decision = m_ai->ChooseMove(context);
        if (decision.valid) {
            PerformAIMove(me, decision.target, decision.cost, is1P);
        }
    }

    void BattleMaster::ExecuteBattle(UnitBase& attacker, UnitBase& defender, UnitBase& target) {
        if (m_rule.IsRoundBattle()) {
            ExecuteRoundCalculation(
                attacker,
                defender.GetNumber(),
                false,
                IntVector2{ -1, -1 });
            return;
        }

        char aOp = attacker.GetOp();
        int aNum = attacker.GetNumber();
        int dNum = defender.GetNumber();

        m_effectIntensity = 2.0f;

        if (aOp == '/' && dNum != 0) {
            IntVector2 nodePos = m_rule.GetWarpGrid(aNum, dNum);
            if (!target.HasWarpNode(nodePos)) {
                target.AddWarpNode(nodePos);
                std::string targetName = (&target == m_player.get()) ? "1P" : "2P";
                AddLog("【ワープ】 " + targetName + " が座標 (" + std::to_string(aNum) + ", " + std::to_string(dNum) + ") にワープ設置！");
                ProceduralAudio::GetInstance().PlayPowerSE(9);
            }
        }

        BattleCalculationResult calc = m_rule.CalculateBattleResult(aNum, dNum, aOp);
        std::string aName = (&attacker == m_player.get()) ? "1P" : "2P";

        if (aOp != '/') {
            std::string eqStr = std::to_string(aNum) + " " + std::string(1, aOp) + " " + std::to_string(dNum) + " = " + std::to_string(calc.intValue);
            AddLog("【計算】 " + aName + " が計算を実行！ [ " + eqStr + " ]");
            ProceduralAudio::GetInstance().PlayPowerSE(4);
        }
        else {
            if (calc.cleanDivide) {
                std::string eqStr = std::to_string(aNum) + " / " + std::to_string(dNum) + " = " + std::to_string(calc.intValue);
                AddLog("【計算】 " + aName + " が計算を実行！ [ " + eqStr + " ] (割り切れた！)");
            }
            else {
                AddLog("【計算】 " + aName + " が割り算を実行！ (割り切れなかった…)");
            }
            ProceduralAudio::GetInstance().PlayPowerSE(4);
        }

        bool is1P = (&attacker == m_player.get());
        if (is1P) {
            m_p1TotalOps++;
            if (std::abs(calc.intValue) > m_p1MaxDamage) m_p1MaxDamage = std::abs(calc.intValue);
        }
        else {
            m_p2TotalOps++;
            if (std::abs(calc.intValue) > m_p2MaxDamage) m_p2MaxDamage = std::abs(calc.intValue);
        }

        ApplyBattleResult(target, calc.fraction, calc.intValue, aOp, calc.cleanDivide);
        // ラウンドバトルではドラフトした演算子を継続保持する。
        // 盤面演算子を拾った場合はその演算子へ置換されるため、常に戦術選択肢を残せる。
        if (!m_rule.IsRoundBattle()) {
            attacker.SetOp('\0');
        }
        AddLog("----------------------------------------");
    }

    void BattleMaster::ApplyBattleResult(UnitBase& unit, const Fraction& resultFrac, int intRes, char op, bool isCleanDivide) {
        std::string targetName = (&unit == m_player.get()) ? "1P" : "2P";

        if (m_rule.IsRoundBattle()) {
            const bool targetIs1P = (&unit == m_player.get());
            Fraction& currentScore = targetIs1P ? m_p1RoundScore : m_p2RoundScore;
            BattleSimulatedUnitState next = m_rule.SimulateRoundBattleResult(
                unit.GetNumber(), unit.GetStocks(), currentScore, m_roundTarget,
                intRes, resultFrac, op, isCleanDivide);

            if (!next.valid) {
                AddLog("【反映】 割り切れなかったため、ラウンド合計値 / パワーは変化しません。");
                return;
            }

            const Fraction previous = currentScore;
            const Fraction raw = previous + resultFrac;
            currentScore = next.score;
            const Fraction goal(m_roundTarget);

            if (raw > goal) {
                AddLog("【合計値】 " + targetName + " : " + previous.ToString() + " -> " + raw.ToString());
                AddLog("【跳ね返り】 目標値超過 -> " + currentScore.ToString());
            }
            else {
                AddLog("【合計値】 " + targetName + " : " + previous.ToString() + " -> " + currentScore.ToString());
            }

            unit.SetNumber(next.number);
            AddLog("【パワー】 " + targetName + " -> " + std::to_string(next.number));
            ProceduralAudio::GetInstance().PlayPowerSE(next.number);

            if (m_rule.IsRoundTargetReached(currentScore, m_roundTarget)) {
                AddLog("【目標値到達】 " + targetName + " が " + std::to_string(m_roundTarget) + " に到達！");
                EndRound(targetIs1P ? 1 : 2);
            }
            return;
        }

        if (m_rule.IsZeroOne()) {
            Fraction& currentScore = (&unit == m_player.get()) ? m_p1ZeroOneScore : m_p2ZeroOneScore;
            BattleSimulatedUnitState next = m_rule.SimulateZeroOneResult(
                unit.GetNumber(), unit.GetStocks(), currentScore, intRes, resultFrac, op, isCleanDivide);

            if (!next.valid) {
                AddLog("【反映】 割り切れなかったため、スコアの加算とパワーの変動はスキップされました。");
                return;
            }

            Fraction previous = currentScore;
            Fraction predicted = previous + resultFrac;
            Fraction goal(m_rule.GetTargetScore());
            currentScore = next.score;

            if (predicted == goal) {
                AddLog("【反映】 " + targetName + " のスコアに適用！ [" + previous.ToString() + " -> " + currentScore.ToString() + "]");
                AddLog("【ぴったり!!】 " + targetName + " が目標スコアにピッタリ到達！！");
            }
            else if (predicted > goal) {
                AddLog("【反映】 " + targetName + " のスコアに適用！ [" + previous.ToString() + " -> " + predicted.ToString() + "]");
                AddLog("【オーバー】 目標を超過！スコアが [" + currentScore.ToString() + "] までバウンス");
            }
            else {
                AddLog("【反映】 " + targetName + " のスコアに適用！ [" + previous.ToString() + " -> " + currentScore.ToString() + "]");
            }

            AddLog("【設定】 " + targetName + " のパワーが [" + std::to_string(next.number) + "] に再設定されました。");
            unit.SetNumber(next.number);
            ProceduralAudio::GetInstance().PlayPowerSE(next.number);
            return;
        }

        BattleSimulatedUnitState next = m_rule.SimulateClassicResult(
            unit.GetNumber(), unit.GetStocks(), unit.GetMaxStocks(), intRes, op, isCleanDivide);

        if (!next.valid) {
            AddLog("【反映】 割り切れなかったため、ダメージ処理はスキップされました。");
            return;
        }

        if (next.defeated) {
            SetClassicDefeat(unit, "エネルギー枯渇");
            ProceduralAudio::GetInstance().PlayErrorSE();
            return;
        }

        if (next.stockDelta < 0) {
            unit.AddStocks(next.stockDelta);
            AddLog("【負荷】 " + targetName + " のバッテリーが " + std::to_string(std::abs(next.stockDelta)) + " 減少！");
            ProceduralAudio::GetInstance().PlayErrorSE();
        }
        else if (next.stockDelta > 0) {
            unit.AddStocks(next.stockDelta);
            AddLog("【充電】 " + targetName + " のバッテリーが " + std::to_string(next.stockDelta) + " 回復！");
            ProceduralAudio::GetInstance().PlayPowerSE(8);
        }
        else {
            AddLog("【適用】 " + targetName + " の数値を書き換え");
        }

        unit.SetNumber(next.number);
        AddLog("【着地】 " + targetName + " のパワーは [" + std::to_string(next.number) + "] に変更されました");
        ProceduralAudio::GetInstance().PlayPowerSE(next.number);
    }

    void BattleMaster::Draw() const {
        if (!m_ui) return;
        BattleViewData view = BuildBattleViewData();
        m_ui->Draw(view);
    }

    bool BattleMaster::IsGameOver() const {
        if (!m_player || !m_enemy) return false;
        return m_rule.IsGameOver(
            m_p1ZeroOneScore,
            m_p2ZeroOneScore,
            m_isBattleFinished);
    }

    bool BattleMaster::IsPlayerWin() const {
        if (!m_player || !m_enemy) return false;

        bool is1PWin = m_rule.IsP1Winner(
            m_p1ZeroOneScore,
            m_p2ZeroOneScore,
            m_is1PWinner);

        // オンラインではクライアント視点に変換する。
        bool isOnline = (NetworkManager::GetInstance() != nullptr &&
            NetworkManager::GetInstance()->GetState() != NetworkManager::State::OFFLINE);
        if (isOnline) {
            bool isHost = NetworkManager::GetInstance()->IsHost();
            if (!isHost) return !is1PWin;
        }
        return is1PWin;
    }

    bool BattleMaster::CheckButtonClick(int x, int y, int w, int h, const Vector2& mousePos) const {
        return (mousePos.x >= static_cast<float>(x) && mousePos.x <= static_cast<float>(x + w) &&
            mousePos.y >= static_cast<float>(y) && mousePos.y <= static_cast<float>(y + h));
    }

    void BattleMaster::AddPowerWithBattery(UnitBase& unit, int delta, const std::string& reason) {
        const std::string targetName = (&unit == m_player.get()) ? "1P" : "2P";

        // ラウンドバトルのSTOCKは残機。移動では一切増減させない。
        if (m_rule.IsRoundBattle()) {
            const BattleSimulatedUnitState next = m_rule.SimulateRoundMoveCost(
                unit.GetNumber(), unit.GetStocks(), -delta);
            const int before = unit.GetNumber();
            unit.SetNumber(next.number);
            AddLog("【パワー】 " + targetName + " " + reason + " : " +
                std::to_string(before) + " -> " + std::to_string(next.number));
            return;
        }

        // delta は「加算量」なので、移動コストシミュレーションには -delta を渡す。
        const BattleSimulatedUnitState next = m_rule.SimulateMoveCost(
            unit.GetNumber(),
            unit.GetStocks(),
            unit.GetMaxStocks(),
            -delta);

        if (next.defeated) {
            SetClassicDefeat(unit, reason);
            return;
        }

        if (next.stockDelta < 0) {
            unit.AddStocks(next.stockDelta);
            AddLog("【消費】 " + targetName + " は " + reason + " によりバッテリーを 1 消費！");
        }
        else if (next.stockDelta > 0) {
            unit.AddStocks(next.stockDelta);
            AddLog("【充電】 " + targetName + " は " + reason + " によりバッテリーを 1 回復！");
        }

        unit.SetNumber(next.number);
    }

} // namespace App