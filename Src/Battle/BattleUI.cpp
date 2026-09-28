#define NOMINMAX
#include <DxLib.h>
#include "BattleUI.h"
#include "BattleViewData.h"
#include "BattleUICommon.h"
#include "../Manager/TutorialMaster.h"
#include "../Shader/CyberGrid.h"
#include "../Utility/AppConfig.h"
#include "../Object/Map/MapGrid.h"
#include "../Object/Unit/UnitBase.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace App {
    using namespace Config;
    using namespace BattleUIDraw;

    BattleUI::BattleUI()
        : m_psHandle(-1)
        , m_cbHandle(-1)
        , m_shaderTime(0.0f)
        , m_uiCursorX_1P(0.0f)
        , m_uiCursorX_2P(0.0f) {
    }

    BattleUI::~BattleUI() {
        if (m_psHandle != -1) DeleteShader(m_psHandle);
        if (m_cbHandle != -1) DeleteShaderConstantBuffer(m_cbHandle);
    }

    void BattleUI::Init() {
        m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));
        m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        m_shaderTime = 0.0f;
        m_uiCursorX_1P = 0.0f;
        m_uiCursorX_2P = 0.0f;

        m_logUI.Init();
        m_commUI.Init();
    }

    void BattleUI::AddLog(const std::string& message) {
        m_logUI.AddLog(message);
    }

    void BattleUI::ScrollLog(int wheelDelta, float mouseX, float mouseY) {
        m_logUI.Scroll(wheelDelta, mouseX, mouseY);
    }

    void BattleUI::Update(float effectIntensity, int p1Num, int p2Num) {
        auto updateCursor = [](float& currentX, int targetNum) {
            const float targetX = 40.0f + (targetNum - 1) * 48.0f;
            if (currentX == 0.0f) currentX = targetX;
            currentX += (targetX - currentX) * 0.2f;
            };

        updateCursor(m_uiCursorX_1P, p1Num);
        updateCursor(m_uiCursorX_2P, p2Num);

        m_shaderTime += 0.0008f + (0.01f * effectIntensity);
        m_commUI.Update();
    }

    int BattleUI::GetCachedFont(int size) {
        return BattleUIDraw::GetCachedFont(size);
    }

    void BattleUI::DrawBackground() const {
        if (m_psHandle != -1 && m_cbHandle != -1) {
            float* cb = static_cast<float*>(GetBufferShaderConstantBuffer(m_cbHandle));
            cb[0] = m_shaderTime;
            cb[1] = static_cast<float>(SCREEN_W);
            cb[2] = static_cast<float>(SCREEN_H);
            cb[3] = 0.0f;

            UpdateShaderConstantBuffer(m_cbHandle);
            SetShaderConstantBuffer(m_cbHandle, DX_SHADERTYPE_PIXEL, 0);
            SetUsePixelShader(m_psHandle);

            VERTEX2DSHADER v[6];
            for (int i = 0; i < 6; ++i) {
                v[i].pos = VGet(0, 0, 0);
                v[i].rhw = 1.0f;
                v[i].dif = GetColorU8(255, 255, 255, 255);
                v[i].spc = GetColorU8(0, 0, 0, 0);
                v[i].u = 0.0f;
                v[i].v = 0.0f;
            }

            v[0].pos.x = 0;        v[0].pos.y = 0;        v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].pos.x = SCREEN_W; v[1].pos.y = 0;        v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].pos.x = 0;        v[2].pos.y = SCREEN_H; v[2].u = 0.0f; v[2].v = 1.0f;
            v[3].pos.x = SCREEN_W; v[3].pos.y = 0;        v[3].u = 1.0f; v[3].v = 0.0f;
            v[4].pos.x = SCREEN_W; v[4].pos.y = SCREEN_H; v[4].u = 1.0f; v[4].v = 1.0f;
            v[5].pos.x = 0;        v[5].pos.y = SCREEN_H; v[5].u = 0.0f; v[5].v = 1.0f;

            DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
            SetUsePixelShader(-1);
        }
        else {
            DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_BG(), TRUE);
        }
    }

    void BattleUI::DrawHeader(const BattleViewData& view) const {
        unsigned int phaseCol = COL_INFO();
        const char* phaseName = "終了！！";

        if (view.roundSetupActive) {
            phaseCol = GetColor(25, 80, 110);
            phaseName = "ラウンド準備";
        }
        else if (!view.gameOver && view.phase != BattleViewPhase::FINISH) {
            if (view.phase == BattleViewPhase::P1_TurnStart || view.phase == BattleViewPhase::P1_Move) {
                phaseCol = GetColor(180, 110, 0);
                phaseName = view.p1.isNPC ? "1Pのターン (思考中)" : "1Pのターン (移動選択)";
            }
            else if (view.phase == BattleViewPhase::P1_Action) {
                phaseCol = GetColor(180, 110, 0);
                phaseName = view.p1.isNPC ? "1Pのターン (思考中)" : "1Pのターン (行動選択)";
            }
            else if (view.phase == BattleViewPhase::P2_TurnStart || view.phase == BattleViewPhase::P2_Move) {
                phaseCol = GetColor(30, 50, 120);
                phaseName = view.p2.isNPC ? "2Pのターン (思考中)" : "2Pのターン (移動選択)";
            }
            else if (view.phase == BattleViewPhase::P2_Action) {
                phaseCol = GetColor(30, 50, 120);
                phaseName = view.p2.isNPC ? "2Pのターン (思考中)" : "2Pのターン (行動選択)";
            }
        }

        DrawBox(0, 0, SCREEN_W, HEADER_H, phaseCol, TRUE);
        DrawLine(0, HEADER_H, SCREEN_W, HEADER_H, COL_TEXT_MAIN(), 2);
        DrawFormatStringToHandle(40, 16, COL_TEXT_MAIN(), GetCachedFont(38), ">>> %s", phaseName);
        DrawFormatStringToHandle(800, 24, COL_TEXT_SUB(), GetCachedFont(24), "経過ターン: %d", view.totalTurns);
    }

    void BattleUI::DrawTurnStartCutIn(const BattleViewData& view) const {
        if (view.roundSetupActive || view.gameOver ||
            (view.phase != BattleViewPhase::P1_TurnStart && view.phase != BattleViewPhase::P2_TurnStart)) {
            return;
        }

        const bool is1P = view.phase == BattleViewPhase::P1_TurnStart;
        const int timer = view.turnStartTimer;
        const int darkAlpha = timer > 60 ? (80 - timer) * 7 : (timer < 20 ? timer * 7 : 140);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, darkAlpha);
        DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        constexpr int bandH = 160;
        const unsigned int bandCol = is1P ? COL_P1() : COL_P2();
        float scale = 1.0f;
        if (timer > 70) scale = (80 - timer) / 10.0f;
        else if (timer < 10) scale = std::max(0.0f, timer / 10.0f);

        const int currentH = static_cast<int>(bandH * scale);
        const int currentY = SCREEN_H / 2 - currentH / 2;

        if (currentH > 0) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
            DrawBox(0, currentY, SCREEN_W, currentY + currentH, GetColor(15, 20, 25), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawLine(0, currentY, SCREEN_W, currentY, bandCol, 3);
            DrawLine(0, currentY + currentH, SCREEN_W, currentY + currentH, bandCol, 3);
        }

        if (timer <= 75 && timer >= 5) {
            const int f80 = GetCachedFont(80);
            const std::string text = is1P
                ? (view.p1.isNPC ? "1P（コンピューター）のターン" : "1Pのターン")
                : (view.p2.isNPC ? "2P（コンピューター）のターン" : "2Pのターン");
            const int tw = GetDrawStringWidthToHandle(text.c_str(), static_cast<int>(text.length()), f80);
            const int textX = SCREEN_W / 2 - tw / 2 + (40 - timer);
            const int textY = SCREEN_H / 2 - 40;

            SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
            DrawStringToHandle(textX - 4, textY, text.c_str(), bandCol, f80);
            DrawStringToHandle(textX + 4, textY, text.c_str(), bandCol, f80);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawStringToHandle(textX, textY, text.c_str(), COL_TEXT_MAIN(), f80);
        }
    }

    void BattleUI::DrawPauseButton(const BattleViewData& view) const {
        DrawCyberButton(
            SCREEN_W - 180,
            10,
            160,
            50,
            "ポーズ（Escキー）",
            COL_WARN(),
            view.hoverPauseButton,
            GetCachedFont(22)
        );
    }

    void BattleUI::DrawFinishOverlay(const BattleViewData& view) const {
        if (view.phase != BattleViewPhase::FINISH) return;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_TEXT_DARK(), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        const int f160 = GetCachedFont(160);
        const char* finishText = "ゲームセット";
        const int textW = GetDrawStringWidthToHandle(finishText, static_cast<int>(std::strlen(finishText)), f160);
        DrawStringToHandle(SCREEN_W / 2 - textW / 2, SCREEN_H / 2 - 100, finishText, COL_TEXT_MAIN(), f160);

        if (view.finishTimer > 30) {
            const int f40 = GetCachedFont(40);
            const char* nextText = ">> クリックで進む <<";
            const int nw = GetDrawStringWidthToHandle(nextText, static_cast<int>(std::strlen(nextText)), f40);
            DrawStringToHandle(SCREEN_W / 2 - nw / 2, SCREEN_H / 2 + 100, nextText, COL_TEXT_SUB(), f40);
        }
    }


    void BattleUI::DrawRoundBattleControls(const BattleViewData& view) const {
        if (view.ruleMode != BattleViewRuleMode::ROUND_BATTLE ||
            view.roundSetupActive ||
            view.phase == BattleViewPhase::FINISH) {
            return;
        }

        const bool is1P = view.is1PTurn;
        const BattleUnitView& unit = is1P ? view.p1 : view.p2;
        if (unit.isNPC) return;

        const unsigned int accent = is1P ? COL_P1() : COL_P2();
        const char fixedOp = is1P ? view.p1FixedOperator : view.p2FixedOperator;
        const char subOp = is1P ? view.p1SubOperator : view.p2SubOperator;
        const char turnOp = is1P ? view.p1TurnOperator : view.p2TurnOperator;
        const bool usingSub = is1P ? view.p1UsingSub : view.p2UsingSub;

        if (view.roundChipPlacementPending) {
            if (view.map) {
                for (const auto& pos : view.roundChipPlacementCells) {
                    const Vector2 center = view.map->GetCellCenter(pos.x, pos.y);
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 85);
                    DrawBox(
                        static_cast<int>(center.x) - 36,
                        static_cast<int>(center.y) - 36,
                        static_cast<int>(center.x) + 36,
                        static_cast<int>(center.y) + 36,
                        COL_SAFE(), TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
                    DrawBox(
                        static_cast<int>(center.x) - 39,
                        static_cast<int>(center.y) - 39,
                        static_cast<int>(center.x) + 39,
                        static_cast<int>(center.y) + 39,
                        COL_SAFE(), FALSE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
            }

            DrawCyberPanel(650, 815, 620, 150, GetColor(8, 18, 24), COL_SAFE(), 245);
            DrawStringToHandle(
                690, 840,
                ("[" + std::to_string(view.roundPendingResult) + "] 数字チップを置くマスをクリック").c_str(),
                COL_TEXT_MAIN(),
                GetCachedFont(30));
            DrawStringToHandle(
                690, 900,
                "自分の周囲8マス / アイテム・駒があるマスは不可",
                COL_TEXT_SUB(),
                GetCachedFont(20));
            return;
        }

        if (view.roundResultPending) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
            DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(0, 0, 0), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            DrawCyberPanel(570, 390, 780, 390, GetColor(8, 18, 28), accent, 250);
            DrawStringToHandle(650, 435, "計算結果", accent, GetCachedFont(34));
            DrawFormatStringToHandle(
                650, 500, COL_TEXT_MAIN(), GetCachedFont(54),
                "%d  ->  [%d]",
                view.roundPendingRaw,
                view.roundPendingResult);

            DrawStringToHandle(
                650, 575,
                "この結果をどう使う？",
                COL_TEXT_SUB(),
                GetCachedFont(26));

            const unsigned int totalCol = view.roundCanAddTotal ? COL_SAFE() : COL_DISABLE();
            DrawCyberButton(
                700, 650, 240, 90,
                view.roundCanAddTotal ? "合計値へ加算" : "合計値（超過）",
                totalCol,
                view.hoverRoundTotalButton && view.roundCanAddTotal,
                GetCachedFont(28));

            DrawCyberButton(
                980, 650, 240, 90,
                "数字チップ化",
                COL_INFO(),
                view.hoverRoundChipButton,
                GetCachedFont(28));
            return;
        }

        const bool movePhase =
            view.phase == BattleViewPhase::P1_Move ||
            view.phase == BattleViewPhase::P2_Move;
        const bool actionPhase =
            view.phase == BattleViewPhase::P1_Action ||
            view.phase == BattleViewPhase::P2_Action;

        if (movePhase) {
            DrawCyberPanel(590, 820, 740, 120, GetColor(7, 14, 24), accent, 240);
            DrawStringToHandle(610, 828, "このターンの演算子", COL_TEXT_SUB(), GetCachedFont(18));

            std::string fixedText = "固定演算子  [";
            fixedText += fixedOp == '\0' ? '-' : fixedOp;
            fixedText += "]";
            DrawCyberButton(
                610, 850, 330, 70,
                fixedText.c_str(),
                !usingSub ? accent : COL_TEXT_SUB(),
                view.hoverRoundFixedButton,
                GetCachedFont(28));

            std::string subText = "サブ演算子  [";
            subText += subOp == '\0' ? '-' : subOp;
            subText += "]";
            DrawCyberButton(
                980, 850, 330, 70,
                subText.c_str(),
                subOp != '\0' ? COL_INFO() : COL_DISABLE(),
                view.hoverRoundSubButton && subOp != '\0',
                GetCachedFont(28));

            DrawFormatStringToHandle(
                610, 930, COL_TEXT_SUB(), GetCachedFont(18),
                "選択中 [%c]  / サブ演算子は使用したターンで消費",
                turnOp == '\0' ? '-' : turnOp);
        }

        if (actionPhase) {
            DrawCyberPanel(590, 920, 740, 120, GetColor(7, 14, 24), accent, 245);

            DrawCyberButton(
                610, 960, 210, 60,
                "相手と計算",
                view.roundPlayerCalcAvailable ? accent : COL_DISABLE(),
                view.hoverRoundPlayerCalcButton && view.roundPlayerCalcAvailable,
                GetCachedFont(22));

            std::string chipText = view.roundChipCalcAvailable
                ? "チップ [" + std::to_string(view.roundChipOperand) + "] と計算"
                : "チップなし";
            DrawCyberButton(
                855, 960, 210, 60,
                chipText.c_str(),
                view.roundChipCalcAvailable ? COL_SAFE() : COL_DISABLE(),
                view.hoverRoundChipCalcButton && view.roundChipCalcAvailable,
                GetCachedFont(22));

            DrawCyberButton(
                1100, 960, 220, 60,
                "行動終了",
                COL_TEXT_SUB(),
                view.hoverNoActionButton,
                GetCachedFont(22));
        }
    }

    void BattleUI::Draw(const BattleViewData& view) const {
        if (!view.map) return;

        DrawBackground();

        // 左右の大枠はBattleUI全体のレイアウトなのでここに残す。
        DrawCyberPanel(0, HEADER_H, 580, SCREEN_H - HEADER_H, COL_PANEL_BG(), COL_P1(), 180);
        DrawCyberPanel(1340, HEADER_H, SCREEN_W - 1340, SCREEN_H - HEADER_H, COL_PANEL_BG(), COL_P2(), 180);

        m_boardUI.Draw(view);
        DrawHeader(view);
        m_statusUI.Draw(view, m_uiCursorX_1P, m_uiCursorX_2P);
        m_logUI.Draw();

        DrawTurnStartCutIn(view);
        DrawPauseButton(view);
        DrawFinishOverlay(view);
        DrawRoundBattleControls(view);
        m_roundSetupUI.Draw(view);

        m_commUI.Draw();
    }

    // ==========================================
    // チュートリアル用UI
    // 現段階ではTutorialMasterとの既存結合を維持する。
    // ==========================================
    void BattleUI::Draw(const TutorialMaster& master) const {
        DrawBackground();

        DrawCyberPanel(0, HEADER_H, 580, SCREEN_H - HEADER_H, COL_PANEL_BG(), COL_P1(), 180);
        DrawCyberPanel(1340, HEADER_H, SCREEN_W - 1340, SCREEN_H - HEADER_H, COL_PANEL_BG(), COL_P2(), 180);

        master.m_mapGrid.Draw();
        if (master.m_player) master.m_player->Draw();
        if (master.m_enemy) master.m_enemy->Draw();

        DrawBox(0, 0, SCREEN_W, HEADER_H, COL_INFO(), TRUE);
        DrawLine(0, HEADER_H, SCREEN_W, HEADER_H, COL_TEXT_MAIN(), 2);
        DrawStringToHandle(40, 16, ">>> チュートリアル", COL_TEXT_MAIN(), GetCachedFont(38));

        auto drawSimpleUnitCard = [&](int x, int y, UnitBase* unit, bool is1P) {
            if (!unit) return;

            const unsigned int baseCol = is1P ? COL_P1() : COL_P2();
            const std::string headerName = is1P ? "1P（あなた）" : "2P（相手）";

            DrawCyberPanel(x, y, 500, 490, GetColor(15, 18, 25), baseCol, 220);
            DrawStringToHandle(x + 15, y + 10, headerName.c_str(), baseCol, GetCachedFont(36));
            DrawLine(x + 10, y + 50, x + 490, y + 50, baseCol, 2);

            const int scoreY = y + 60;

            if (master.m_ruleMode == 1) {
                DrawCyberPanel(x + 10, scoreY, 480, 130, COL_DARK_BG(), baseCol, 255);
                DrawStringToHandle(x + 20, scoreY + 5, "現在のスコア", COL_TEXT_SUB(), GetCachedFont(18));

                const int displayScore = is1P ? static_cast<int>(master.m_p1DisplayScore) : static_cast<int>(master.m_p2DisplayScore);
                const int f100 = GetCachedFont(100);
                const std::string nStr = std::to_string(displayScore);
                DrawStringToHandle(x + 34, scoreY + 24, nStr.c_str(), COL_TEXT_DARK(), f100);
                DrawStringToHandle(x + 30, scoreY + 20, nStr.c_str(), COL_TEXT_MAIN(), f100);

                const int targetBoxY = scoreY + 140;
                DrawCyberPanel(x + 10, targetBoxY, 480, 40, GetColor(20, 30, 40), COL_INFO(), 255);
                DrawStringToHandle(x + 20, targetBoxY + 10, "目標スコア", COL_INFO(), GetCachedFont(20));
                DrawFormatStringToHandle(x + 390, targetBoxY + 4, COL_TEXT_DARK(), GetCachedFont(32), "000");
                DrawFormatStringToHandle(x + 390, targetBoxY + 4, COL_SAFE(), GetCachedFont(32), "%03d", master.m_targetScore);
            }
            else {
                DrawCyberPanel(x + 10, scoreY, 480, 40, COL_DARK_BG(), COL_TEXT_SUB(), 255);
                DrawStringToHandle(x + 20, scoreY + 10, "ノーマルバトル", COL_TEXT_SUB(), GetCachedFont(20));
            }

            const int powerY = scoreY + (master.m_ruleMode == 1 ? 190 : 50);
            DrawCyberPanel(x + 10, powerY, 480, 110, COL_DARK_BG(), baseCol, 255);
            DrawStringToHandle(x + 20, powerY + 10, "パワー", COL_WARN(), GetCachedFont(24));
            DrawLine(x + 100, powerY + 24, x + 480, powerY + 24, GetColor(60, 60, 70), 1);

            DrawBox(x + 15, powerY + 40, x + 485, powerY + 90, GetColor(10, 10, 15), TRUE);
            DrawLine(x + 15, powerY + 65, x + 485, powerY + 65, GetColor(30, 30, 40), 1);

            const int currentNum = unit->GetNumber();
            const int f32Num = GetCachedFont(32);
            for (int i = 1; i <= 9; ++i) {
                const int px = x + 40 + (i - 1) * 48;
                const int py = powerY + 43;
                const unsigned int numCol = i == currentNum ? COL_SAFE() : GetColor(70, 70, 80);
                const std::string numStr = std::to_string(i);
                const int tw = GetDrawStringWidthToHandle(numStr.c_str(), 1, f32Num);
                DrawStringToHandle(px - tw / 2, py + (i == currentNum ? -10 : 6), numStr.c_str(), numCol, f32Num);
            }

            const int infoY = powerY + 120;
            DrawCyberPanel(x + 10, infoY, 480, 110, COL_DARK_BG(), baseCol, 255);

            if (master.m_ruleMode == 0) {
                DrawStringToHandle(x + 20, infoY + 15, "バッテリー", GetColor(180, 180, 180), GetCachedFont(22));
                DrawBatteryGauge(
                    x + 140,
                    infoY + 12,
                    unit->GetStocks(),
                    unit->GetStocks(),
                    unit->GetMaxStocks(),
                    COL_SAFE(),
                    COL_DANGER(),
                    baseCol
                );
            }

            const int moveDist = 3 - ((unit->GetNumber() - 1) % 3);
            DrawFormatStringToHandle(x + 20, infoY + 65, COL_TEXT_SUB(), GetCachedFont(22), "移動可能: %d マス", moveDist);

            const int ix = x + 350;
            const int iy = infoY + 15;
            DrawStringToHandle(ix - 5, iy, "【移動範囲】", COL_DISABLE(), GetCachedFont(18));
            const int targetNumForGrid = unit->GetNumber();

            for (int i = 0; i < 9; ++i) {
                const int gx = i % 3;
                const int gy = i / 3;
                const bool canGo = i == 4
                    ? false
                    : (targetNumForGrid <= 3
                        ? (gx == 1 || gy == 1)
                        : (targetNumForGrid <= 6 ? (gx == gy || gx + gy == 2) : true));
                const unsigned int dotCol = canGo ? baseCol : GetColor(40, 40, 50);
                DrawBox(ix + gx * 22, iy + 26 + gy * 22, ix + gx * 22 + 18, iy + 26 + gy * 22 + 18, dotCol, TRUE);
            }
            };

        drawSimpleUnitCard(40, 100, master.m_player.get(), true);
        drawSimpleUnitCard(1380, 100, master.m_enemy.get(), false);

        m_logUI.Draw();

        DrawCyberPanel(600, BOTTOM_PANEL_Y, 720, 160, COL_BOTTOM_BG(), COL_INFO(), 220);
        DrawStringToHandle(
            620,
            BOTTOM_PANEL_Y + 60,
            "中央のメッセージウィンドウの指示に従って操作してください。",
            COL_TEXT_MAIN(),
            GetCachedFont(22)
        );
    }

} // namespace App
