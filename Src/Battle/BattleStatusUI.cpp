#define NOMINMAX
#include <DxLib.h>
#include "BattleStatusUI.h"
#include "BattleViewData.h"
#include "BattleUICommon.h"
#include "../Utility/AppConfig.h"

#include <algorithm>
#include <cmath>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace App {
    using namespace Config;
    using namespace BattleUIDraw;

    namespace {
        std::string FractionToString(const FractionView& value) {
            if (value.d == 1) return std::to_string(value.n);
            return std::to_string(value.n) + "/" + std::to_string(value.d);
        }
    }

    void BattleStatusUI::DrawUnitCard(
        int x,
        int y,
        const BattleViewData& view,
        const BattleUnitView& unitView,
        bool is1P,
        float cursorX
    ) const {
        if (!unitView.unit) return;

        const unsigned int baseCol = is1P ? COL_P1() : COL_P2();
        const std::string headerName = is1P
            ? (unitView.isNPC ? "1P (COM)" : "1P PLAYER")
            : (unitView.isNPC ? "2P (COM)" : "2P PLAYER");

        if (unitView.activeTurn) {
            SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(80 + 40 * std::sin(GetNowCount() / 200.0)));
            DrawBox(x, y, x + 500, y + 490, baseCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        DrawCyberPanel(x, y, 500, 490, GetColor(15, 18, 25), baseCol, 220);
        DrawStringToHandle(x + 15, y + 10, headerName.c_str(), baseCol, GetCachedFont(36));
        DrawLine(x + 10, y + 50, x + 490, y + 50, baseCol, 2);

        const int scoreY = y + 60;
        const bool hasScorePanel = view.ruleMode == BattleViewRuleMode::ZERO_ONE ||
            view.ruleMode == BattleViewRuleMode::ROUND_BATTLE;
        if (hasScorePanel) {
            DrawCyberPanel(x + 10, scoreY, 480, 130, COL_DARK_BG(), baseCol, 255);
            DrawStringToHandle(
                x + 20, scoreY + 5,
                view.ruleMode == BattleViewRuleMode::ROUND_BATTLE ? "ROUND SCORE" : "現在のスコア",
                COL_TEXT_SUB(), GetCachedFont(18));

            if (unitView.score.d == 1) {
                const int f100 = GetCachedFont(100);
                const std::string nStr = std::to_string(unitView.displayScore);
                DrawStringToHandle(x + 34, scoreY + 24, nStr.c_str(), COL_TEXT_DARK(), f100);
                DrawStringToHandle(x + 30, scoreY + 20, nStr.c_str(), COL_TEXT_MAIN(), f100);
            }
            else {
                const unsigned int fracCol = is1P ? GetColor(255, 220, 100) : GetColor(180, 220, 255);
                const int f60 = GetCachedFont(60);
                const std::string nStr = std::to_string(unitView.displayScore);
                const std::string dStr = std::to_string(unitView.score.d);
                const int nw = GetDrawStringWidthToHandle(nStr.c_str(), static_cast<int>(nStr.length()), f60);
                const int dw = GetDrawStringWidthToHandle(dStr.c_str(), static_cast<int>(dStr.length()), f60);
                const int maxW = std::max(nw, dw);
                const int cx = x + 110;

                DrawFormatStringToHandle(cx - nw / 2 + 4, scoreY + 16, COL_TEXT_DARK(), f60, "%s", nStr.c_str());
                DrawFormatStringToHandle(cx - nw / 2, scoreY + 12, fracCol, f60, "%s", nStr.c_str());
                DrawLine(cx - maxW / 2 - 10, scoreY + 75, cx + maxW / 2 + 10, scoreY + 75, fracCol, 4);
                DrawFormatStringToHandle(cx - dw / 2 + 4, scoreY + 84, COL_TEXT_DARK(), f60, "%s", dStr.c_str());
                DrawFormatStringToHandle(cx - dw / 2, scoreY + 80, fracCol, f60, "%s", dStr.c_str());
            }

            if (unitView.hasScorePreview &&
                (unitView.previewScore.n != unitView.score.n || unitView.previewScore.d != unitView.score.d)) {
                const int f60 = GetCachedFont(60);
                const int f40 = GetCachedFont(40);
                const unsigned int previewCol =
                    (unitView.previewScore.n == view.targetScore && unitView.previewScore.d == 1)
                    ? COL_WARN()
                    : (unitView.previewScore.n * unitView.score.d < unitView.score.n * unitView.previewScore.d
                        ? COL_DANGER()
                        : COL_SAFE());
                const int arrowX = x + 240;

                SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(150 + 100 * std::sin(GetNowCount() / 100.0)));
                DrawStringToHandle(arrowX, scoreY + 45, "→", COL_TEXT_SUB(), f40);

                if (unitView.previewScore.d == 1) {
                    const std::string pStr = std::to_string(unitView.previewScore.n);
                    DrawStringToHandle(arrowX + 54, scoreY + 34, pStr.c_str(), COL_TEXT_DARK(), f60);
                    DrawStringToHandle(arrowX + 50, scoreY + 30, pStr.c_str(), previewCol, f60);
                }
                else {
                    const int f30 = GetCachedFont(30);
                    DrawFormatStringToHandle(arrowX + 60, scoreY + 20, previewCol, f30, "%lld", unitView.previewScore.n);
                    DrawLine(arrowX + 55, scoreY + 60, arrowX + 110, scoreY + 60, previewCol, 3);
                    DrawFormatStringToHandle(arrowX + 60, scoreY + 65, previewCol, f30, "%lld", unitView.previewScore.d);
                }

                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }

            const int targetBoxY = scoreY + 140;
            DrawCyberPanel(x + 10, targetBoxY, 480, 40, GetColor(20, 30, 40), COL_INFO(), 255);
            DrawStringToHandle(
                x + 20, targetBoxY + 10,
                view.ruleMode == BattleViewRuleMode::ROUND_BATTLE ? "ROUND TARGET" : "目標スコア",
                COL_INFO(), GetCachedFont(20));
            DrawFormatStringToHandle(x + 390, targetBoxY + 4, COL_SAFE(), GetCachedFont(32), "%03d", view.targetScore);
        }
        else {
            DrawCyberPanel(x + 10, scoreY, 480, 40, COL_DARK_BG(), COL_TEXT_SUB(), 255);
            const char* modeName = view.ruleMode == BattleViewRuleMode::ROUND_BATTLE
                ? "ラウンドバトル"
                : "ノーマルバトル";
            DrawStringToHandle(x + 20, scoreY + 10, modeName, COL_TEXT_SUB(), GetCachedFont(20));
        }

        const int powerY = scoreY + (hasScorePanel ? 190 : 50);
        DrawCyberPanel(x + 10, powerY, 480, 110, COL_DARK_BG(), baseCol, 255);
        DrawStringToHandle(x + 20, powerY + 10, "パワー", COL_WARN(), GetCachedFont(24));
        DrawLine(x + 100, powerY + 24, x + 480, powerY + 24, GetColor(60, 60, 70), 1);

        const int currentNum = unitView.number;
        const int previewNum = unitView.hasPowerPreview ? unitView.previewNumber : currentNum;
        const int previewStocks = unitView.hasPowerPreview ? unitView.previewStocks : unitView.stocks;
        if (cursorX == 0.0f) cursorX = 40.0f + (currentNum - 1) * 48.0f;

        DrawBox(x + 15, powerY + 40, x + 485, powerY + 90, GetColor(10, 10, 15), TRUE);
        DrawLine(x + 15, powerY + 65, x + 485, powerY + 65, GetColor(30, 30, 40), 1);

        const bool isDeadPreview =
            unitView.hasPowerPreview && unitView.previewDefeated && view.ruleMode == BattleViewRuleMode::CLASSIC;
        const unsigned int frameCol = isDeadPreview ? COL_DANGER() : baseCol;
        const int hx = x + static_cast<int>(cursorX);
        const int hy = powerY + 29;
        constexpr int fw = 52;
        constexpr int fh = 66;

        SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
        DrawBox(hx - fw / 2, hy, hx + fw / 2, hy + fh, frameCol, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        const int f32Num = GetCachedFont(32);
        const int f16Tri = GetCachedFont(16);
        for (int i = 1; i <= 9; ++i) {
            const int px = x + 40 + (i - 1) * 48;
            const int py = powerY + 43;
            const std::string numStr = std::to_string(i);
            const float targetX = 40.0f + (i - 1) * 48.0f;
            const float distance = std::abs(cursorX - targetX);
            float currentYOff = 6.0f;
            unsigned int numCol = GetColor(70, 70, 80);

            if (distance < 48.0f && !isDeadPreview) {
                float ratio = 1.0f - (distance / 48.0f);
                ratio = std::sin(ratio * static_cast<float>(M_PI) / 2.0f);
                currentYOff = 6.0f - (20.0f * ratio);
                numCol = (ratio > 0.8f) ? COL_TEXT_MAIN() : COL_TEXT_SUB();
            }

            if (unitView.hasPowerPreview && i == previewNum && previewNum != currentNum && !isDeadPreview) {
                currentYOff = -14.0f;
                numCol = COL_SAFE();
                const int twTriangle = GetDrawStringWidthToHandle("▲", 2, f16Tri);
                DrawStringToHandle(px - twTriangle / 2, py + 26, "▲", COL_SAFE(), f16Tri);
            }

            const int tw = GetDrawStringWidthToHandle(numStr.c_str(), 1, f32Num);
            DrawStringToHandle(px - tw / 2, py + static_cast<int>(currentYOff), numStr.c_str(), numCol, f32Num);
        }

        const int infoY = powerY + 120;
        DrawCyberPanel(x + 10, infoY, 480, 110, COL_DARK_BG(), baseCol, 255);
        const int f22 = GetCachedFont(22);

        if (view.ruleMode == BattleViewRuleMode::CLASSIC || view.ruleMode == BattleViewRuleMode::ROUND_BATTLE) {
            const char* stockLabel = view.ruleMode == BattleViewRuleMode::ROUND_BATTLE ? "STOCK" : "バッテリー";
            DrawStringToHandle(x + 20, infoY + 15, stockLabel, GetColor(180, 180, 180), f22);
            DrawBatteryGauge(
                x + 140,
                infoY + 12,
                unitView.stocks,
                previewStocks,
                unitView.maxStocks,
                COL_SAFE(),
                COL_DANGER(),
                baseCol
            );
        }

        if (unitView.hasPowerPreview && !unitView.previewDefeated && previewNum != currentNum) {
            DrawFormatStringToHandle(
                x + 20,
                infoY + 65,
                COL_TEXT_SUB(),
                f22,
                "移動可能: %d → %d マス",
                unitView.moveDistance,
                unitView.previewMoveDistance
            );
        }
        else {
            DrawFormatStringToHandle(
                x + 20,
                infoY + 65,
                COL_TEXT_SUB(),
                f22,
                "移動可能: %d マス",
                unitView.moveDistance
            );
        }

        const int ix = x + 350;
        const int iy = infoY + 15;
        DrawStringToHandle(ix - 5, iy, "【移動範囲】", COL_DISABLE(), GetCachedFont(18));

        const auto& dots = (unitView.hasPowerPreview && !unitView.previewDefeated)
            ? unitView.previewMoveDirectionDots
            : unitView.moveDirectionDots;
        const unsigned int gridBaseCol = unitView.hasPowerPreview ? COL_SAFE() : baseCol;

        for (int i = 0; i < 9; ++i) {
            const int gx = i % 3;
            const int gy = i / 3;
            const unsigned int dotCol = dots[i] ? gridBaseCol : GetColor(40, 40, 50);
            DrawBox(ix + gx * 22, iy + 26 + gy * 22, ix + gx * 22 + 18, iy + 26 + gy * 22 + 18, dotCol, TRUE);
        }
    }

    void BattleStatusUI::DrawRulePanel(const BattleViewData& view) const {
        const int f22 = GetCachedFont(22);
        const int f20 = GetCachedFont(20);

        DrawCyberPanel(1380, LOG_PANEL_Y, 500, 200, COL_PANEL_BG(), GetColor(100, 100, 120), 150);
        DrawStringToHandle(1395, LOG_PANEL_Y + 10, "■ 基本ルール", COL_DISABLE(), f22);
        DrawLine(1390, LOG_PANEL_Y + 35, 1870, LOG_PANEL_Y + 35, GetColor(60, 60, 70), 1);

        for (int i = 0; i < static_cast<int>(view.ruleLines.size()); ++i) {
            DrawStringToHandle(
                1395,
                LOG_PANEL_Y + 45 + i * 30,
                view.ruleLines[i].c_str(),
                i == 4 ? COL_INFO() : COL_TEXT_SUB(),
                f20
            );
        }
    }

    void BattleStatusUI::DrawCalculationPanel(const BattleViewData& view) const {
        const unsigned int calcBorderCol = view.is1PTurn ? COL_P1() : COL_P2();
        DrawCyberPanel(600, BOTTOM_PANEL_Y, 720, 160, COL_BOTTOM_BG(), calcBorderCol, 220);

        const BattleCalculationView& calc = view.calculation;
        if (calc.visible) {
            const int f64 = GetCachedFont(64);
            const int f22 = GetCachedFont(22);
            const int f20 = GetCachedFont(20);
            const int f16 = GetCachedFont(16);

            if (calc.movePreview) {
                const int preY = BOTTOM_PANEL_Y - 26;
                if (calc.willGetNewOperator) {
                    DrawFormatStringToHandle(
                        600,
                        preY,
                        GetColor(255, 200, 100),
                        f20,
                        "▼ 移動プレビュー(演算子 [%c] を取得してバトル)",
                        calc.op
                    );
                }
                else {
                    DrawStringToHandle(600, preY, "▼ 移動プレビュー(バトル発生)", GetColor(255, 200, 100), f20);
                }
            }

            const std::string leftLabel = calc.op == '/' ? "X座標" : "自分";
            const std::string rightLabel = calc.op == '/' ? "Y座標" : "相手";
            const unsigned int leftCol = calc.op == '/' ? COL_INFO() : (view.is1PTurn ? COL_P1() : COL_P2());
            const unsigned int rightCol = calc.op == '/' ? COL_INFO() : (view.is1PTurn ? COL_P2() : COL_P1());
            const int calcY = BOTTOM_PANEL_Y + 22;

            DrawStringToHandle(662, calcY - 18, leftLabel.c_str(), leftCol, f16);
            DrawStringToHandle(750, calcY - 18, rightLabel.c_str(), rightCol, f16);
            DrawFormatStringToHandle(662, calcY, COL_TEXT_DARK(), f64, "%d %c %d =", calc.leftNumber, calc.op, calc.rightNumber);
            DrawFormatStringToHandle(660, calcY - 2, COL_TEXT_MAIN(), f64, "%d %c %d =", calc.leftNumber, calc.op, calc.rightNumber);

            const unsigned int resColor = calc.intResult < 0
                ? COL_DANGER()
                : (!calc.cleanDivide
                    ? COL_DISABLE()
                    : (view.ruleMode != BattleViewRuleMode::CLASSIC ? COL_WARN() : COL_SAFE()));
            const int resX = view.ruleMode != BattleViewRuleMode::CLASSIC ? 1000 : 1020;
            DrawFormatStringToHandle(resX + 2, calcY, COL_TEXT_DARK(), f64, "%d", calc.intResult);
            DrawFormatStringToHandle(resX, calcY - 2, resColor, f64, "%d", calc.intResult);

            auto drawTargetResult = [&](int x, int y, const char* label, unsigned int labelCol, const BattleTargetResultView& result) {
                DrawStringToHandle(x, y, label, labelCol, f22);

                if (!result.valid) {
                    DrawStringToHandle(x + 140, y, "数値変動なし", COL_DISABLE(), f20);
                    return;
                }

                if (result.defeated) {
                    DrawStringToHandle(x + 140, y, "撃破！", COL_WARN(), f22);
                    return;
                }

                if (result.hasScore) {
                    const std::string scoreText = FractionToString(result.score);
                    DrawFormatStringToHandle(
                        x + 140,
                        y,
                        COL_TEXT_SUB(),
                        f20,
                        "スコア: %s / パワー:%d",
                        scoreText.c_str(),
                        result.number
                    );
                }
                else {
                    DrawFormatStringToHandle(x + 140, y, COL_TEXT_SUB(), f20, "残機 %d | パワー %d", result.stocks, result.number);
                }
                };

            if (calc.movePreview) {
                drawTargetResult(630, BOTTOM_PANEL_Y + 95, "【自分に反映】", COL_SAFE(), calc.selfResult);
                drawTargetResult(970, BOTTOM_PANEL_Y + 95, "【相手に反映】", COL_DANGER(), calc.enemyResult);

                if (calc.createsWarp) {
                    DrawFormatStringToHandle(
                        630,
                        BOTTOM_PANEL_Y + 125,
                        COL_INFO(),
                        f20,
                        "選択した対象に座標(%d, %d)のワープを設置",
                        calc.warpDisplay.x,
                        calc.warpDisplay.y
                    );
                }
            }
            else if (calc.hoverSelf || calc.hoverEnemy) {
                const BattleTargetResultView& selected = calc.hoverSelf ? calc.selfResult : calc.enemyResult;
                const char* targetName = calc.hoverSelf
                    ? (view.is1PTurn ? "1P" : "2P")
                    : (view.is1PTurn ? "2P" : "1P");

                if (!selected.valid) {
                    DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 92, COL_DISABLE(), f22, "▼ %s : 数値変動なし", targetName);
                }
                else if (selected.defeated) {
                    DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 92, COL_DANGER(), f22, "▼ %s を撃破", targetName);
                }
                else if (selected.hasScore) {
                    const std::string scoreText = FractionToString(selected.score);
                    DrawFormatStringToHandle(
                        630,
                        BOTTOM_PANEL_Y + 92,
                        COL_TEXT_MAIN(),
                        f22,
                        "▼ %s : スコア %s / パワー %d",
                        targetName,
                        scoreText.c_str(),
                        selected.number
                    );
                }
                else if (selected.stockDelta < 0) {
                    DrawFormatStringToHandle(
                        630,
                        BOTTOM_PANEL_Y + 92,
                        COL_DANGER(),
                        f22,
                        "▼ %s : バッテリー %d / パワー %d",
                        targetName,
                        selected.stockDelta,
                        selected.number
                    );
                }
                else if (selected.stockDelta > 0) {
                    DrawFormatStringToHandle(
                        630,
                        BOTTOM_PANEL_Y + 92,
                        COL_SAFE(),
                        f22,
                        "▼ %s : バッテリー +%d / パワー %d",
                        targetName,
                        selected.stockDelta,
                        selected.number
                    );
                }
                else {
                    DrawFormatStringToHandle(
                        630,
                        BOTTOM_PANEL_Y + 92,
                        COL_TEXT_SUB(),
                        f22,
                        "▼ %s : パワー %d",
                        targetName,
                        selected.number
                    );
                }

                if (calc.createsWarp) {
                    DrawFormatStringToHandle(
                        630,
                        BOTTOM_PANEL_Y + 122,
                        calc.hoverSelf ? COL_SAFE() : COL_DANGER(),
                        f20,
                        "さらに座標(%d, %d)に【%s】のワープ設置！",
                        calc.warpDisplay.x,
                        calc.warpDisplay.y,
                        targetName
                    );
                }
            }
            else {
                DrawStringToHandle(730, BOTTOM_PANEL_Y + 92, "反映する対象を選択してください", GetColor(120, 120, 130), f22);
                if (calc.createsWarp) {
                    DrawStringToHandle(730, BOTTOM_PANEL_Y + 122, "実行時、選択した対象にワープを設置！", COL_INFO(), f20);
                }
            }
        }
        else if (view.phase == BattleViewPhase::P1_Action || view.phase == BattleViewPhase::P2_Action) {
            const int f28 = GetCachedFont(28);
            if (view.canAttack && !view.hasOperator) {
                DrawStringToHandle(730, BOTTOM_PANEL_Y + 60, "【 演算子アイテムが必要です 】", COL_DANGER(), f28);
            }
            else {
                DrawStringToHandle(770, BOTTOM_PANEL_Y + 60, "ターゲットが射程内にいません", COL_DISABLE(), f28);
            }
        }
        else {
            DrawStringToHandle(750, BOTTOM_PANEL_Y + 60, "移動するマスを選択してください", COL_DISABLE(), GetCachedFont(28));
        }
    }

    void BattleStatusUI::DrawActionButtons(const BattleViewData& view) const {
        if (!view.humanActionPhase) return;

        constexpr int by = 960;
        const int f26 = GetCachedFont(26);
        const BattleCalculationView& calc = view.calculation;

        if (view.canAttack && view.hasOperator) {
            DrawCyberButton(600, by, 220, 60, "自分", COL_SAFE(), calc.hoverSelf, f26);
            DrawCyberButton(850, by, 220, 60, "相手", COL_DANGER(), calc.hoverEnemy, f26);
            DrawCyberButton(1100, by, 220, 60, "何もしない", COL_DISABLE(), view.hoverNoActionButton, f26);
        }
        else {
            DrawCyberButton(750, by, 420, 60, "ターン終了", GetColor(180, 180, 200), view.hoverEndTurnButton, f26);
        }
    }

    void BattleStatusUI::Draw(const BattleViewData& view, float cursor1P, float cursor2P) const {
        DrawUnitCard(40, 100, view, view.p1, true, cursor1P);
        DrawUnitCard(1380, 100, view, view.p2, false, cursor2P);
        DrawRulePanel(view);
        DrawCalculationPanel(view);
        DrawActionButtons(view);
    }

} // namespace App
