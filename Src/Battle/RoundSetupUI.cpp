#define NOMINMAX
#include <DxLib.h>

#include "RoundSetupUI.h"
#include "BattleViewData.h"
#include "BattleUICommon.h"

#include <string>

namespace App {
    using namespace BattleUIDraw;

    namespace {
        constexpr int SCREEN_W = 1920;
        constexpr int SCREEN_H = 1080;

        unsigned int COL_TEXT_MAIN() { return GetColor(235, 248, 255); }
        unsigned int COL_TEXT_SUB() { return GetColor(160, 185, 205); }
        unsigned int COL_P1() { return GetColor(255, 165, 60); }
        unsigned int COL_P2() { return GetColor(70, 165, 255); }
        unsigned int COL_SAFE() { return GetColor(100, 255, 170); }
        unsigned int COL_DIM() { return GetColor(65, 80, 95); }

        void DrawCenteredText(int y, const std::string& text, unsigned int color, int font) {
            const int w = GetDrawStringWidthToHandle(text.c_str(), static_cast<int>(text.size()), font);
            DrawStringToHandle(SCREEN_W / 2 - w / 2, y, text.c_str(), color, font);
        }

        void DrawNumberChoices(const BattleViewData& view, int y, unsigned int accent, int font) {
            constexpr int BOX_W = 86;
            constexpr int BOX_H = 86;
            constexpr int GAP = 16;
            constexpr int COUNT = 9;
            const int totalW = BOX_W * COUNT + GAP * (COUNT - 1);
            const int startX = (SCREEN_W - totalW) / 2;

            int mx = 0, my = 0;
            GetMousePoint(&mx, &my);

            for (int i = 0; i < COUNT; ++i) {
                const int number = i + 1;
                const int x = startX + i * (BOX_W + GAP);
                const bool hover = mx >= x && mx <= x + BOX_W && my >= y && my <= y + BOX_H;
                const bool selected = (number == view.roundNumberCursor) || hover;
                const unsigned int edge = selected ? accent : GetColor(65, 90, 115);
                const unsigned int base = selected ? GetColor(20, 45, 60) : GetColor(8, 15, 28);

                SetDrawBlendMode(DX_BLENDMODE_ALPHA, selected ? 245 : 210);
                DrawBox(x, y, x + BOX_W, y + BOX_H, base, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                DrawBox(x, y, x + BOX_W, y + BOX_H, edge, FALSE);
                if (selected) DrawBox(x + 4, y + 4, x + BOX_W - 4, y + BOX_H - 4, accent, FALSE);

                const std::string label = std::to_string(number);
                const int tw = GetDrawStringWidthToHandle(label.c_str(), static_cast<int>(label.size()), font);
                DrawStringToHandle(x + (BOX_W - tw) / 2, y + 15, label.c_str(), selected ? COL_TEXT_MAIN() : COL_TEXT_SUB(), font);
            }
        }

        void DrawOperatorChoices(const BattleViewData& view, int y, unsigned int accent, int font) {
            constexpr int BOX_W = 160;
            constexpr int BOX_H = 120;
            constexpr int GAP = 42;
            const int totalW = BOX_W * 4 + GAP * 3;
            const int startX = (SCREEN_W - totalW) / 2;

            int mx = 0, my = 0;
            GetMousePoint(&mx, &my);

            for (int i = 0; i < 4; ++i) {
                const int x = startX + i * (BOX_W + GAP);
                const bool available = view.roundOperatorAvailable[i];
                const bool hover = mx >= x && mx <= x + BOX_W && my >= y && my <= y + BOX_H;
                const bool selected = available && (i == view.roundOperatorCursor || hover);
                const unsigned int edge = selected ? accent : (available ? GetColor(85, 115, 145) : COL_DIM());
                const unsigned int base = available ? GetColor(10, 22, 38) : GetColor(14, 16, 20);

                SetDrawBlendMode(DX_BLENDMODE_ALPHA, available ? 240 : 150);
                DrawBox(x, y, x + BOX_W, y + BOX_H, base, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                DrawBox(x, y, x + BOX_W, y + BOX_H, edge, FALSE);
                if (selected) DrawBox(x + 4, y + 4, x + BOX_W - 4, y + BOX_H - 4, accent, FALSE);

                const std::string op(1, view.roundOperators[i]);
                const int tw = GetDrawStringWidthToHandle(op.c_str(), 1, font);
                DrawStringToHandle(x + (BOX_W - tw) / 2, y + 10, op.c_str(), available ? (selected ? COL_TEXT_MAIN() : COL_TEXT_SUB()) : COL_DIM(), font);

                const std::string key = "[" + std::to_string(i + 1) + "]";
                const int kw = GetDrawStringWidthToHandle(key.c_str(), static_cast<int>(key.size()), GetCachedFont(20));
                DrawStringToHandle(x + (BOX_W - kw) / 2, y + 88, key.c_str(), available ? COL_TEXT_SUB() : COL_DIM(), GetCachedFont(20));
            }
        }

        std::string OpText(char op) {
            return op == '\0' ? "-" : std::string(1, op);
        }

        std::string GridText(const IntVector2& pos) {
            if (pos.x < 0 || pos.y < 0) return "(-,-)";
            return "(" + std::to_string(pos.x + 1) + "," + std::to_string(9 - pos.y) + ")";
        }
    }

    void RoundSetupUI::Draw(const BattleViewData& view) const {
        if (!view.roundSetupActive) return;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 225);
        DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(3, 8, 18), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        const int panelW = 1220;
        const int panelH = 650;
        const int panelX = (SCREEN_W - panelW) / 2;
        const int panelY = (SCREEN_H - panelH) / 2;

        unsigned int accent = GetColor(120, 220, 255);
        if (view.roundPhase == BattleRoundViewPhase::SELECT_P1_NUMBER || view.roundPhase == BattleRoundViewPhase::DRAFT_P1_OPERATOR) accent = COL_P1();
        if (view.roundPhase == BattleRoundViewPhase::SELECT_P2_NUMBER || view.roundPhase == BattleRoundViewPhase::DRAFT_P2_OPERATOR) accent = COL_P2();
        if (view.roundPhase == BattleRoundViewPhase::TARGET_REVEAL) accent = COL_SAFE();
        if (view.roundPhase == BattleRoundViewPhase::ROUND_END) accent = view.roundWinner == 1 ? COL_P1() : COL_P2();

        DrawCyberPanel(panelX, panelY, panelW, panelH, GetColor(10, 18, 32), accent, 245);

        const int f72 = GetCachedFont(72);
        const int f64 = GetCachedFont(64);
        const int f52 = GetCachedFont(52);
        const int f48 = GetCachedFont(48);
        const int f36 = GetCachedFont(36);
        const int f30 = GetCachedFont(30);
        const int f28 = GetCachedFont(28);
        const int f22 = GetCachedFont(22);

        DrawCenteredText(panelY + 32, "ラウンド " + std::to_string(view.roundNumber), accent, f72);

        switch (view.roundPhase) {
        case BattleRoundViewPhase::ROUND_START:
            DrawCenteredText(panelY + 175, "ラウンド開始", COL_TEXT_MAIN(), f52);
            DrawCenteredText(panelY + 285, "P1 / P2 が 1-9 から初期数字を選択", COL_TEXT_SUB(), f30);
            DrawCenteredText(panelY + 390, "目標値 = 9 + P1 + P2", COL_SAFE(), f48);
            {
                const int bx = 760;
                const int by = 720;
                const int bw = 400;
                const int bh = 70;
                int mx = 0, my = 0;
                GetMousePoint(&mx, &my);
                const bool hover = mx >= bx && mx <= bx + bw && my >= by && my <= by + bh;
                DrawCyberButton(bx, by, bw, bh, "開始", accent, hover, f30);
                DrawCenteredText(panelY + 590, "クリック / スペース / エンター", COL_TEXT_SUB(), f22);
            }
            break;

        case BattleRoundViewPhase::SELECT_P1_NUMBER:
        case BattleRoundViewPhase::SELECT_P2_NUMBER:
        {
            const bool is1P = view.roundPhase == BattleRoundViewPhase::SELECT_P1_NUMBER;
            const bool isNPC = is1P ? view.p1.isNPC : view.p2.isNPC;
            DrawCenteredText(panelY + 140, is1P ? "1P 初期数字選択" : "2P 初期数字選択", COL_TEXT_MAIN(), f48);
            if (view.roundWaitingForRemote) {
                DrawCenteredText(panelY + 310, "相手が選択中...", accent, f48);
            }
            else if (isNPC) {
                DrawCenteredText(panelY + 310, "コンピューター思考中...", accent, f52);
            }
            else {
                DrawNumberChoices(view, panelY + 250, accent, f48);
                DrawCenteredText(panelY + 390, "1〜9をクリックして選択", COL_TEXT_SUB(), f28);
            }
            if (!is1P && view.p1RoundStartNumber > 0) {
                DrawCenteredText(panelY + 485, "1Pの選択 : " + std::to_string(view.p1RoundStartNumber), COL_P1(), f30);
            }
            break;
        }

        case BattleRoundViewPhase::TARGET_REVEAL:
        {
            DrawCenteredText(panelY + 145, "初期数字 決定", COL_TEXT_MAIN(), f48);
            DrawCenteredText(panelY + 235,
                "1P : " + std::to_string(view.p1RoundStartNumber) + "        2P : " + std::to_string(view.p2RoundStartNumber),
                COL_TEXT_SUB(), f36);
            const std::string formula = "目標値 = 9 + " + std::to_string(view.p1RoundStartNumber) +
                " + " + std::to_string(view.p2RoundStartNumber) + " = " + std::to_string(view.roundTarget);
            DrawCenteredText(panelY + 335, formula, COL_SAFE(), f64);
            DrawCenteredText(panelY + 500, "次へ：演算子ドラフト", COL_TEXT_SUB(), f28);
            break;
        }

        case BattleRoundViewPhase::DRAFT_P1_OPERATOR:
        case BattleRoundViewPhase::DRAFT_P2_OPERATOR:
        {
            const bool is1P = view.roundPhase == BattleRoundViewPhase::DRAFT_P1_OPERATOR;
            const bool isNPC = is1P ? view.p1.isNPC : view.p2.isNPC;
            DrawCenteredText(panelY + 135, is1P ? "1P 演算子ドラフト" : "2P 演算子ドラフト", COL_TEXT_MAIN(), f48);
            if (!is1P) {
                DrawCenteredText(panelY + 200, "1Pの選択 : [" + OpText(view.p1DraftedOperator) + "]", COL_P1(), f28);
            }
            if (view.roundWaitingForRemote) {
                DrawCenteredText(panelY + 335, "相手が演算子を選択中...", accent, f48);
            }
            else if (isNPC) {
                DrawCenteredText(panelY + 335, "コンピューター思考中...", accent, f52);
            }
            else {
                DrawOperatorChoices(view, panelY + 275, accent, f64);
                DrawCenteredText(panelY + 455, "選択できる演算子をクリック", COL_TEXT_SUB(), f28);
            }
            DrawCenteredText(panelY + 535, "各プレイヤーが1個取得 / 残り2個は盤面へ", COL_TEXT_SUB(), f22);
            break;
        }

        case BattleRoundViewPhase::PLACE_OPERATORS:
        {
            DrawCenteredText(panelY + 135, "演算子配置 完了", COL_TEXT_MAIN(), f48);
            DrawCenteredText(panelY + 225,
                "1P固定演算子 [" + OpText(view.p1DraftedOperator) + "]      2P固定演算子 [" + OpText(view.p2DraftedOperator) + "]",
                COL_TEXT_SUB(), f36);
            DrawCenteredText(panelY + 330,
                "盤面 [" + OpText(view.roundPlacedOperator1) + "]  ->  " + GridText(view.roundPlacedPos1),
                COL_SAFE(), f36);
            DrawCenteredText(panelY + 390,
                "盤面 [" + OpText(view.roundPlacedOperator2) + "]  ->  " + GridText(view.roundPlacedPos2),
                COL_SAFE(), f36);
            DrawCenteredText(panelY + 500, "目標値 " + std::to_string(view.roundTarget) + " / バトル開始", accent, f36);
            break;
        }

        case BattleRoundViewPhase::ROUND_END:
        {
            const bool matchDecided = view.p1RoundWins >= 2 || view.p2RoundWins >= 2;
            DrawCenteredText(panelY + 145, "ラウンド " + std::to_string(view.roundNumber) + " の勝者", COL_TEXT_SUB(), f36);
            DrawCenteredText(panelY + 215, std::to_string(view.roundWinner) + "P", accent, f72);
            DrawCenteredText(panelY + 330, "目標値 " + std::to_string(view.roundTarget) + " にぴったり到達！", COL_SAFE(), f48);
            DrawCenteredText(panelY + 420,
                "ラウンド勝利   1P " + std::to_string(view.p1RoundWins) +
                "  -  " + std::to_string(view.p2RoundWins) + " 2P",
                COL_TEXT_MAIN(), f36);
            DrawCenteredText(panelY + 515,
                matchDecided ? "試合決着" : "次のラウンド",
                matchDecided ? accent : COL_TEXT_SUB(), f30);
            break;
        }

        default:
            break;
        }
    }
}
