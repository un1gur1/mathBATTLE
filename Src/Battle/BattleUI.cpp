#define NOMINMAX
#include <DxLib.h>
#include "BattleUI.h"
#include "../Manager/BattleMaster.h"
#include "../Manager/TutorialMaster.h"
#include "../Input/InputManager.h"
#include "../Shader/CyberGrid.h"
#include "../Utility/AppConfig.h" 
#include <string>
#include "../Object/Map/MapGrid.h"
#include <unordered_map>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace App {
    using namespace Config;

    // ==========================================
    // UI描画用のサイバーデザイン・ヘルパー群（無名名前空間）
    // ==========================================
    namespace {
        // サイバーなホログラムパネルを描画する関数
        void DrawCyberPanel(int x, int y, int w, int h, unsigned int baseCol, unsigned int edgeCol, int alpha = 200) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
            DrawBox(x, y, x + w, y + h, baseCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // 四隅のロックオンブラケット（L字のアクセント）
            int cl = 12;
            DrawLine(x, y, x + cl, y, edgeCol, 2);
            DrawLine(x, y, x, y + cl, edgeCol, 2);
            DrawLine(x + w - cl, y, x + w, y, edgeCol, 2);
            DrawLine(x + w, y, x + w, y + cl, edgeCol, 2);
            DrawLine(x, y + h, x + cl, y + h, edgeCol, 2);
            DrawLine(x, y + h - cl, x, y + h, edgeCol, 2);
            DrawLine(x + w - cl, y + h, x + w, y + h, edgeCol, 2);
            DrawLine(x + w, y + h - cl, x + w, y + h, edgeCol, 2);

            // 上下の細いスキャンライン
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
            DrawLine(x + cl, y, x + w - cl, y, edgeCol, 1);
            DrawLine(x + cl, y + h, x + w - cl, y + h, edgeCol, 1);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // サイバーなボタンを描画する関数
        void DrawCyberButton(int x, int y, int w, int h, const char* text, unsigned int col, bool isHover, int fontHandle) {
            if (isHover) {
                SetDrawBlendMode(DX_BLENDMODE_ADD, 180);
                DrawBox(x, y, x + w, y + h, col, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                DrawBox(x, y, x + w, y + h, COL_WHITE(), FALSE);
                int tw = GetDrawStringWidthToHandle(text, (int)strlen(text), fontHandle);
                DrawStringToHandle(x + (w - tw) / 2, y + (h - 26) / 2, text, COL_TEXT_DARK(), fontHandle);
            }
            else {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
                DrawBox(x, y, x + w, y + h, GetColor(15, 20, 25), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                DrawBox(x, y, x + w, y + h, col, FALSE);
                int tw = GetDrawStringWidthToHandle(text, (int)strlen(text), fontHandle);
                DrawStringToHandle(x + (w - tw) / 2, y + (h - 26) / 2, text, col, fontHandle);
            }
        }

        // バッテリーゲージを斜めスリット状のサイバーデザインで描画する関数
        void DrawBatteryGauge(int x, int y, int current, int preview, int maxStocks, unsigned int safeCol, unsigned int dangerCol, unsigned int baseCol) {
            int bw = 35; // 1ブロックの幅
            int bh = 24; // ブロックの高さ
            int gap = 8; // ブロック間の隙間
            int f16 = BattleUI::GetCachedFont(16);

            for (int i = 0; i < maxStocks; ++i) {
                int bx = x + i * (bw + gap);
                int by = y;

                // 平行四辺形の頂点を計算（斜めカット）
                int slant = 8;
                int px[4] = { bx + slant, bx + bw, bx + bw - slant, bx };
                int py[4] = { by, by, by + bh, by + bh };

                auto drawQuad = [&](unsigned int color, bool fill) {
                    DrawQuadrangleAA((float)px[0], (float)py[0], (float)px[1], (float)py[1], (float)px[2], (float)py[2], (float)px[3], (float)py[3], color, fill);
                    };

                if (i < current && i >= preview) {
                    // 消費予定（点滅赤）
                    int blinkAlpha = (int)(120 + 120 * std::sin(GetNowCount() / 80.0));
                    SetDrawBlendMode(DX_BLENDMODE_ADD, blinkAlpha);
                    drawQuad(dangerCol, TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    drawQuad(GetColor(255, 255, 255), FALSE);
                    DrawStringToHandle(bx - 2, by - 18, "消費", dangerCol, f16);
                }
                else if (i >= current && i < preview) {
                    // 回復予定（点滅緑）
                    int blinkAlpha = (int)(120 + 120 * std::sin(GetNowCount() / 80.0));
                    SetDrawBlendMode(DX_BLENDMODE_ADD, blinkAlpha);
                    drawQuad(safeCol, TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    drawQuad(GetColor(255, 255, 255), FALSE);
                    DrawStringToHandle(bx - 2, by - 18, "回復", safeCol, f16);
                }
                else if (i < current) {
                    // 保持しているバッテリー（メインカラー）
                    drawQuad(baseCol, TRUE);
                    // 内側のハイライト
                    SetDrawBlendMode(DX_BLENDMODE_ADD, 100);
                    DrawQuadrangleAA((float)px[0] + 2, (float)py[0] + 2, (float)px[1] - 2, (float)py[1] + 2, (float)px[2] - 2, (float)py[2] - 2, (float)px[3] + 2, (float)py[3] - 2, COL_WHITE(), TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
                else {
                    // 空の枠
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
                    drawQuad(GetColor(30, 35, 45), TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    drawQuad(GetColor(80, 85, 100), FALSE);
                }
            }
        }
    }

    BattleUI::BattleUI()
        : m_psHandle(-1), m_cbHandle(-1), m_shaderTime(0.0f)
        , m_logScrollOffset(0), m_uiCursorX_1P(0.0f), m_uiCursorX_2P(0.0f) {
    }

    BattleUI::~BattleUI() {
        if (m_psHandle != -1) DeleteShader(m_psHandle);
        if (m_cbHandle != -1) DeleteShaderConstantBuffer(m_cbHandle);
    }

    void BattleUI::Init() {
        m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));
        m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        m_shaderTime = 0.0f;
        m_logScrollOffset = 0;
        m_uiCursorX_1P = 0.0f;
        m_uiCursorX_2P = 0.0f;
        m_actionLog.clear();
        m_commUI.Init();
    }

    void BattleUI::AddLog(const std::string& message) {
        m_actionLog.push_back(message);
        if (m_actionLog.size() > 100) m_actionLog.erase(m_actionLog.begin());
        m_logScrollOffset = std::max(0, (int)m_actionLog.size() - 6);
    }

    void BattleUI::ScrollLog(int wheelDelta, float mouseX, float mouseY) {
        if (mouseX >= 40 && mouseX <= 540 && mouseY >= LOG_PANEL_Y && mouseY <= LOG_PANEL_Y + 200) {
            m_logScrollOffset -= wheelDelta;
            int maxOffset = std::max(0, (int)m_actionLog.size() - 6);
            if (m_logScrollOffset < 0) m_logScrollOffset = 0;
            if (m_logScrollOffset > maxOffset) m_logScrollOffset = maxOffset;
        }
    }

    void BattleUI::Update(float effectIntensity, int p1Num, int p2Num) {
        auto updateCursor = [](float& currentX, int targetNum) {
            float targetX = 40.0f + (targetNum - 1) * 48.0f;
            if (currentX == 0.0f) currentX = targetX;
            currentX += (targetX - currentX) * 0.2f; // スムーズに追従
            };
        updateCursor(m_uiCursorX_1P, p1Num);
        updateCursor(m_uiCursorX_2P, p2Num);
        m_shaderTime += 0.0008f + (0.01f * effectIntensity);
        m_commUI.Update();
    }

    int BattleUI::GetCachedFont(int size) {
        static std::unordered_map<int, int> s_fontCache;
        if (s_fontCache.find(size) == s_fontCache.end()) {
            s_fontCache[size] = CreateFontToHandle("BIZ UDゴシック", size, 2, DX_FONTTYPE_ANTIALIASING);
        }
        return s_fontCache[size];
    }

    // ==========================================
    // 危険エリア（敵の攻撃範囲）描画
    // ==========================================
    void BattleUI::DrawEnemyDangerArea(const BattleMaster& master) const {
        UnitBase* opp = master.GetTargetUnit();
        if (!opp || opp->GetStocks() <= 0 || opp->IsMoving()) return;

        IntVector2 ePos = opp->GetGridPos();
        int eNum = opp->GetNumber();
        char eOp = opp->GetOp();

        double time = GetNowCount() / 2500.0;
        int dangerAlpha = (int)(20 + 15 * sin(time * M_PI * 3.0));
        int dangerFrameAlpha = (int)(100 + 40 * sin(time * M_PI * 3.0));

        for (int x = 0; x < master.m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < master.m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };
                Vector2 center = master.m_mapGrid.GetCellCenter(x, y);

                if (target == ePos) {
                    SetDrawBlendMode(DX_BLENDMODE_ADD, 100);
                    DrawCircleAA(center.x, center.y, 25.0f, 32, COL_DANGER(), FALSE, 2.0f);
                    continue;
                }

                int dummyCost = 0;
                bool canMoveBase = master.CanMove(eNum, '\0', ePos, target, dummyCost);
                bool canMoveCombined = master.CanMove(eNum, eOp, ePos, target, dummyCost);

                if (canMoveCombined) {
                    unsigned int col = canMoveBase ? COL_DANGER_DIM() : COL_WARN();
                    int cx = (int)center.x, cy = (int)center.y, size = 38;

                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, dangerAlpha);
                    DrawBox(cx - size, cy - size, cx + size, cy + size, col, TRUE);

                    SetDrawBlendMode(DX_BLENDMODE_ADD, dangerFrameAlpha);
                    int len = 12;
                    DrawLine(cx - len, cy - size, cx + len, cy - size, col, 2);
                    DrawLine(cx - len, cy + size, cx + len, cy + size, col, 2);
                    DrawLine(cx - size, cy - len, cx - size, cy + len, col, 2);
                    DrawLine(cx + size, cy - len, cx + size, cy + len, col, 2);
                }
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // ==========================================
    // 移動可能エリア描画
    // ==========================================
    void BattleUI::DrawMovableArea(const BattleMaster& master) const {
        if (!master.m_isPlayerSelected) return;
        UnitBase* activeUnit = master.GetActiveUnit();
        if (!activeUnit || activeUnit->IsMoving()) return;

        IntVector2 pPos = activeUnit->GetGridPos();
        int pNum = activeUnit->GetNumber();
        char pOp = activeUnit->GetOp();

        double time = GetNowCount() / 3000.0;
        int pulseAlpha = (int)(30 + 20 * sin(time * M_PI * 4.0));
        int frameAlpha = (int)(150 + 50 * sin(time * M_PI * 4.0));

        for (int x = 0; x < master.m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < master.m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };
                Vector2 center = master.m_mapGrid.GetCellCenter(x, y);

                if (target == pPos) {
                    SetDrawBlendMode(DX_BLENDMODE_ADD, 100);
                    DrawCircleAA(center.x, center.y, 25.0f, 32, COL_P1(), FALSE, 2.0f);
                    continue;
                }

                int dummyCost = 0;
                bool canMoveBase = master.CanMove(pNum, '\0', pPos, target, dummyCost);
                bool canMoveCombined = master.CanMove(pNum, pOp, pPos, target, dummyCost);
                bool isWarpNode = activeUnit->HasWarpNode(target);

                if (canMoveCombined || isWarpNode) {
                    unsigned int col = isWarpNode ? COL_INFO() : (canMoveBase ? GetColor(0, 200, 255) : GetColor(255, 180, 0));
                    int cx = (int)center.x, cy = (int)center.y, size = 40;

                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, pulseAlpha);
                    DrawBox(cx - size, cy - size, cx + size, cy + size, col, TRUE);

                    SetDrawBlendMode(DX_BLENDMODE_ADD, frameAlpha);
                    int len = 8;
                    DrawLine(cx - size, cy - size, cx - size + len, cy - size, col, 2);
                    DrawLine(cx - size, cy - size, cx - size, cy - size + len, col, 2);
                    DrawLine(cx + size - len, cy - size, cx + size, cy - size, col, 2);
                    DrawLine(cx + size, cy - size, cx + size, cy - size + len, col, 2);
                    DrawLine(cx - size, cy + size, cx - size + len, cy + size, col, 2);
                    DrawLine(cx - size, cy + size - len, cx - size, cy + size, col, 2);
                    DrawLine(cx + size - len, cy + size, cx + size, cy + size, col, 2);
                    DrawLine(cx + size, cy + size - len, cx + size, cy + size, col, 2);

                    SetDrawBlendMode(DX_BLENDMODE_ADD, frameAlpha / 2);
                    DrawLine(cx - 3, cy, cx + 3, cy, col, 1);
                    DrawLine(cx, cy - 3, cx, cy + 3, col, 1);
                }
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // マウスホバー時の強調
        if (master.m_mapGrid.IsWithinBounds(master.m_hoverGrid.x, master.m_hoverGrid.y)) {
            int hoverCost = 0;
            bool isHoverSelf = (master.m_hoverGrid == pPos);
            if (isHoverSelf || master.CanMove(pNum, pOp, pPos, master.m_hoverGrid, hoverCost)) {
                Vector2 hc = master.m_mapGrid.GetCellCenter(master.m_hoverGrid.x, master.m_hoverGrid.y);
                int cx = (int)hc.x, cy = (int)hc.y, size = 42;

                SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
                DrawBox(cx - size, cy - size, cx + size, cy + size, COL_WHITE(), FALSE);

                int cl = 6;
                DrawLine(cx - size, cy - size + cl, cx - size + cl, cy - size, COL_WHITE(), 2);
                DrawLine(cx + size - cl, cy - size, cx + size, cy - size + cl, COL_WHITE(), 2);
                DrawLine(cx - size, cy + size - cl, cx - size + cl, cy + size, COL_WHITE(), 2);
                DrawLine(cx + size - cl, cy + size, cx + size, cy + size - cl, COL_WHITE(), 2);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                int displayCost = isHoverSelf ? 1 : hoverCost;
                DrawFormatStringToHandle(cx - 14, cy - 35, COL_DANGER(), GetCachedFont(20), "-%d", displayCost);
            }
        }
    }

    // ==========================================
    // メイン描画処理（バトル本編）
    // ==========================================
    void BattleUI::Draw(const BattleMaster& master) const {
        // 1. 背景シェーダー描画
        if (m_psHandle != -1 && m_cbHandle != -1) {
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            cb[0] = m_shaderTime; cb[1] = SCREEN_W; cb[2] = SCREEN_H; cb[3] = 0.0f;
            UpdateShaderConstantBuffer(m_cbHandle);
            SetShaderConstantBuffer(m_cbHandle, DX_SHADERTYPE_PIXEL, 0);

            SetUsePixelShader(m_psHandle);
            VERTEX2DSHADER v[6];
            for (int i = 0; i < 6; ++i) {
                v[i].pos = VGet(0, 0, 0); v[i].rhw = 1.0f;
                v[i].dif = GetColorU8(255, 255, 255, 255); v[i].spc = GetColorU8(0, 0, 0, 0);
                v[i].u = 0.0f; v[i].v = 0.0f;
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

        // 2. 両サイドのベースパネル
        DrawCyberPanel(0, HEADER_H, 580, SCREEN_H - HEADER_H, COL_PANEL_BG(), COL_P1(), 180);
        DrawCyberPanel(1340, HEADER_H, SCREEN_W - 1340, SCREEN_H - HEADER_H, COL_PANEL_BG(), COL_P2(), 180);

        // 3. マップとエリア描画
        master.m_mapGrid.Draw();

        auto drawWarpNodes = [&](UnitBase* unit, unsigned int baseCol, const char* label) {
            if (!unit) return;
            for (auto pos : unit->GetWarpNodes()) {
                Vector2 center = master.m_mapGrid.GetCellCenter(pos.x, pos.y);
                SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
                DrawCircleAA(center.x, center.y, 42.0f, 64, baseCol, FALSE, 3.0f);
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
                DrawCircleAA(center.x, center.y, 35.0f, 64, baseCol, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                int font18 = GetCachedFont(18);
                int tw = GetDrawStringWidthToHandle(label, (int)strlen(label), font18);
                DrawStringToHandle((int)center.x - tw / 2, (int)center.y + 15, label, COL_TEXT_MAIN(), font18);
            }
            };

        if (master.Is1PTurn()) drawWarpNodes(master.m_player.get(), COL_P1(), "1P ST.");
        else drawWarpNodes(master.m_enemy.get(), COL_P2(), "2P ST.");

        DrawEnemyDangerArea(master);
        DrawMovableArea(master);

        // アクティブユニットの足元ハイライト（盤面固定）
        UnitBase* blinkUnit = master.GetActiveUnit();
        if (blinkUnit && !blinkUnit->IsMoving()) {
            IntVector2 bPos = blinkUnit->GetGridPos();
            Vector2 bCenter = master.GetMapGrid().GetCellCenter(bPos.x, bPos.y);

            double time = GetNowCount() / 1000.0;
            int alpha = (int)(120 + 100 * sin(time * M_PI * 2.0));
            unsigned int auraCol = (blinkUnit == master.m_player.get()) ? COL_WARN() : COL_P2();

            int cx = (int)bCenter.x, cy = (int)bCenter.y, size = 39, len = 12;

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha / 4);
            DrawBox(cx - size, cy - size, cx + size, cy + size, auraCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
            DrawLine(cx - size, cy - size, cx - size + len, cy - size, auraCol, 3);
            DrawLine(cx - size, cy - size, cx - size, cy - size + len, auraCol, 3);
            DrawLine(cx + size - len, cy - size, cx + size, cy - size, auraCol, 3);
            DrawLine(cx + size, cy - size, cx + size, cy - size + len, auraCol, 3);
            DrawLine(cx - size, cy + size, cx - size + len, cy + size, auraCol, 3);
            DrawLine(cx - size, cy + size - len, cx - size, cy + size, auraCol, 3);
            DrawLine(cx + size - len, cy + size, cx + size, cy + size, auraCol, 3);
            DrawLine(cx + size, cy + size - len, cx + size, cy + size, auraCol, 3);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // キャラクター描画
        if (master.m_player) master.m_player->Draw();
        if (master.m_enemy)  master.m_enemy->Draw();

        // 4. ヘッダー描画
        unsigned int phaseCol = COL_INFO();
        const char* phaseName = "終了！！";

        // 勝敗が決まっていなければ、現在のフェーズに合わせて名前を変える
        if (!master.IsGameOver() && master.m_currentPhase != BattleMaster::Phase::FINISH) {
            if (master.m_currentPhase == BattleMaster::Phase::P1_TurnStart || master.m_currentPhase == BattleMaster::Phase::P1_Move) {
                phaseCol = GetColor(180, 110, 0); phaseName = master.m_is1P_NPC ? "1Pのターン (思考中)" : "1Pのターン (移動選択)";
            }
            else if (master.m_currentPhase == BattleMaster::Phase::P1_Action) {
                phaseCol = GetColor(180, 110, 0); phaseName = master.m_is1P_NPC ? "1Pのターン (思考中)" : "1Pのターン (行動選択)";
            }
            else if (master.m_currentPhase == BattleMaster::Phase::P2_TurnStart || master.m_currentPhase == BattleMaster::Phase::P2_Move) {
                phaseCol = GetColor(30, 50, 120); phaseName = master.m_is2P_NPC ? "2Pのターン (思考中)" : "2Pのターン (移動選択)";
            }
            else if (master.m_currentPhase == BattleMaster::Phase::P2_Action) {
                phaseCol = GetColor(30, 50, 120); phaseName = master.m_is2P_NPC ? "2Pのターン (思考中)" : "2Pのターン (行動選択)";
            }
        }

        DrawBox(0, 0, SCREEN_W, HEADER_H, phaseCol, TRUE);
        DrawLine(0, HEADER_H, SCREEN_W, HEADER_H, COL_TEXT_MAIN(), 2);
        DrawFormatStringToHandle(40, 16, COL_TEXT_MAIN(), GetCachedFont(38), ">>> %s", phaseName);
        DrawFormatStringToHandle(800, 24, COL_TEXT_SUB(), GetCachedFont(24), "経過ターン: %d", master.m_mapGrid.GetTotalTurns());


        // ==========================================
        // 事前計算（プレビュー用）
        // ==========================================
        Vector2 mousePos = InputManager::GetInstance().GetMousePos();
        UnitBase* activeActor = master.GetActiveUnit();
        UnitBase* activeTarget = master.GetTargetUnit();

        bool isMovePreview = false, isPreviewNextToEnemy = false, showCalcPanel = false, isPreviewMode = false, willGetNewOp = false;
        int previewCost = 0, previewNum = activeActor ? activeActor->GetNumber() : 0;
        char previewOp = activeActor ? activeActor->GetOp() : '\0';

        if (master.m_isPlayerSelected && activeActor && !activeActor->IsMoving() &&
            (master.m_currentPhase == BattleMaster::Phase::P1_Move || master.m_currentPhase == BattleMaster::Phase::P2_Move)) {
            if (master.m_mapGrid.IsWithinBounds(master.m_hoverGrid.x, master.m_hoverGrid.y)) {
                if (master.m_hoverGrid == activeActor->GetGridPos()) { isMovePreview = true; previewCost = 1; }
                else if (master.CanMove(activeActor->GetNumber(), activeActor->GetOp(), activeActor->GetGridPos(), master.m_hoverGrid, previewCost)) { isMovePreview = true; }

                if (isMovePreview) {
                    previewNum = activeActor->GetNumber() - previewCost;
                    while (previewNum <= 0) previewNum += 9;
                    char itemOnGrid = master.m_mapGrid.GetItemAt(master.m_hoverGrid.x, master.m_hoverGrid.y);
                    if (itemOnGrid != '\0') previewOp = itemOnGrid;

                    if (activeTarget) {
                        IntVector2 tP = activeTarget->GetGridPos();
                        if (std::abs(master.m_hoverGrid.x - tP.x) + std::abs(master.m_hoverGrid.y - tP.y) == 1 && previewOp != '\0') isPreviewNextToEnemy = true;
                    }
                }
            }
        }

        IntVector2 aP = activeActor ? activeActor->GetGridPos() : IntVector2{ -1,-1 };
        IntVector2 tP = activeTarget ? activeTarget->GetGridPos() : IntVector2{ -1,-1 };
        bool canAttack = activeTarget && (std::abs(aP.x - tP.x) + std::abs(aP.y - tP.y) == 1);
        bool hasOp = (activeActor && activeActor->GetOp() != '\0');
        bool isHoverSelf = false, isHoverEnemy = false;

        if (canAttack && hasOp) {
            bool is1PTurn = master.Is1PTurn();
            if (master.CheckButtonClick(600, 960, 220, 60, mousePos)) isHoverSelf = true;
            else if (master.CheckButtonClick(850, 960, 220, 60, mousePos)) isHoverEnemy = true;
            else if (master.CheckButtonClick(40, 100, 500, 650, mousePos)) { if (is1PTurn) isHoverSelf = true; else isHoverEnemy = true; }
            else if (master.CheckButtonClick(1380, 100, 500, 650, mousePos)) { if (is1PTurn) isHoverEnemy = true; else isHoverSelf = true; }
            else if (master.m_hoverGrid == aP) isHoverSelf = true;
            else if (master.m_hoverGrid == tP) isHoverEnemy = true;
        }

        int disp_aNum = 0, disp_tNum = 0; char disp_aOp = '\0';
        if (master.m_currentPhase == BattleMaster::Phase::P1_Action || master.m_currentPhase == BattleMaster::Phase::P2_Action) {
            if (canAttack && hasOp) {
                showCalcPanel = true;
                disp_aNum = activeActor->GetNumber(); disp_tNum = activeTarget->GetNumber(); disp_aOp = activeActor->GetOp();
            }
        }
        else if (master.m_currentPhase == BattleMaster::Phase::P1_Move || master.m_currentPhase == BattleMaster::Phase::P2_Move) {
            if (isMovePreview && isPreviewNextToEnemy && activeTarget && previewOp != '\0') {
                showCalcPanel = true; isPreviewMode = true;
                disp_aNum = previewNum; disp_tNum = activeTarget->GetNumber(); disp_aOp = previewOp;
                if (activeActor->GetOp() != previewOp) willGetNewOp = true;
            }
        }

        int intRes = 0; Fraction resFrac(0); bool isCleanDivide = true;
        if (showCalcPanel) {
            if (disp_aOp == '+') { intRes = disp_aNum + disp_tNum; resFrac = Fraction(intRes); }
            else if (disp_aOp == '-') { intRes = disp_aNum - disp_tNum; resFrac = Fraction(intRes); }
            else if (disp_aOp == '*') { intRes = disp_aNum * disp_tNum; resFrac = Fraction(intRes); }
            else if (disp_aOp == '/') {
                if (disp_tNum != 0 && disp_aNum % disp_tNum == 0) { intRes = disp_aNum / disp_tNum; resFrac = Fraction(intRes); }
                else { isCleanDivide = false; }
            }
        }

        int p1PreviewNum = -1, p1PreviewStocks = master.m_player ? master.m_player->GetStocks() : 0;
        int p2PreviewNum = -1, p2PreviewStocks = master.m_enemy ? master.m_enemy->GetStocks() : 0;
        Fraction p1PreviewScore = master.m_p1ZeroOneScore; bool p1HasScorePreview = false;
        Fraction p2PreviewScore = master.m_p2ZeroOneScore; bool p2HasScorePreview = false;

        if (isMovePreview) {
            if (activeActor == master.m_player.get()) {
                int temp = master.m_player->GetNumber() - previewCost; p1PreviewStocks = master.m_player->GetStocks();
                while (temp <= 0) { p1PreviewStocks--; temp += 9; }
                p1PreviewNum = (p1PreviewStocks < 0) ? 0 : temp;
            }
            else if (activeActor == master.m_enemy.get()) {
                int temp = master.m_enemy->GetNumber() - previewCost; p2PreviewStocks = master.m_enemy->GetStocks();
                while (temp <= 0) { p2PreviewStocks--; temp += 9; }
                p2PreviewNum = (p2PreviewStocks < 0) ? 0 : temp;
            }
        }

        if (showCalcPanel && (isHoverSelf || isHoverEnemy)) {
            bool applyTo1P = (isHoverSelf && activeActor == master.m_player.get()) || (!isHoverSelf && activeTarget == master.m_player.get());
            if (master.m_ruleMode == BattleMaster::RuleMode::CLASSIC) {
                if (disp_aOp != '/') {
                    int currentStocks = applyTo1P ? master.m_player->GetStocks() : master.m_enemy->GetStocks();
                    int simulatedHp = intRes, stockChange = 0;
                    while (simulatedHp <= 0) { stockChange--; simulatedHp += 9; }
                    while (simulatedHp > 9) { stockChange++; simulatedHp -= 9; }
                    int finalStocks = currentStocks + stockChange;
                    int finalNum = (finalStocks < 0) ? 0 : simulatedHp;
                    if (applyTo1P) { p1PreviewNum = finalNum; p1PreviewStocks = finalStocks; }
                    else { p2PreviewNum = finalNum; p2PreviewStocks = finalStocks; }
                }
            }
            else {
                if (disp_aOp != '/') {
                    Fraction goal(master.m_targetScore);
                    Fraction currentScore = applyTo1P ? master.m_p1ZeroOneScore : master.m_p2ZeroOneScore;
                    Fraction nextScore = currentScore + resFrac;
                    if (nextScore > goal) nextScore = goal - (nextScore - goal);
                    if (applyTo1P) { p1PreviewScore = nextScore; p1HasScorePreview = true; }
                    else { p2PreviewScore = nextScore; p2HasScorePreview = true; }

                    int cycleValue = (intRes - 1) % 9;
                    if (cycleValue < 0) cycleValue += 9;
                    if (applyTo1P) p1PreviewNum = cycleValue + 1; else p2PreviewNum = cycleValue + 1;
                }
            }
        }

        // ==========================================
        // 左・右のユニットカード描画
        // ==========================================
        auto drawUnitCard = [&](int x, int y, UnitBase* unit, bool is1P, int p_previewNum, int p_previewStocks, Fraction p_previewScore, bool p_hasScorePreview) {
            if (!unit) return;
            unsigned int baseCol = is1P ? COL_P1() : COL_P2();
            std::string headerName = is1P ? (master.m_is1P_NPC ? "1P (COM)" : "1P PLAYER") : (master.m_is2P_NPC ? "2P (COM)" : "2P PLAYER");

            // アクティブターンの背景ハイライト
            if ((is1P && master.Is1PTurn()) || (!is1P && !master.Is1PTurn() && master.m_currentPhase != BattleMaster::Phase::FINISH)) {
                SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(80 + 40 * sin(GetNowCount() / 200.0)));
                DrawBox(x, y, x + 500, y + 490, baseCol, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }

            // ベースパネル描画
            DrawCyberPanel(x, y, 500, 490, GetColor(15, 18, 25), baseCol, 220);

            // ヘッダー
            DrawStringToHandle(x + 15, y + 10, headerName.c_str(), baseCol, GetCachedFont(36));
            DrawLine(x + 10, y + 50, x + 490, y + 50, baseCol, 2);

            int scoreY = y + 60;
            if (master.m_ruleMode == BattleMaster::RuleMode::ZERO_ONE) {
                DrawCyberPanel(x + 10, scoreY, 480, 130, COL_DARK_BG(), baseCol, 255);
                DrawStringToHandle(x + 20, scoreY + 5, "現在のスコア", COL_TEXT_SUB(), GetCachedFont(18));

                Fraction f = is1P ? master.m_p1ZeroOneScore : master.m_p2ZeroOneScore;
                int displayScore = is1P ? master.GetP1DisplayScore() : master.GetP2DisplayScore();

                if (f.d == 1) {
                    int f100 = GetCachedFont(100);
                    std::string nStr = std::to_string(displayScore);
                    DrawStringToHandle(x + 34, scoreY + 24, nStr.c_str(), COL_TEXT_DARK(), f100);
                    DrawStringToHandle(x + 30, scoreY + 20, nStr.c_str(), COL_TEXT_MAIN(), f100);
                }
                else {
                    unsigned int fracCol = is1P ? GetColor(255, 220, 100) : GetColor(180, 220, 255);
                    int f60 = GetCachedFont(60);
                    std::string nStr = std::to_string(displayScore);
                    std::string dStr = std::to_string(f.d);
                    int nw = GetDrawStringWidthToHandle(nStr.c_str(), (int)nStr.length(), f60);
                    int dw = GetDrawStringWidthToHandle(dStr.c_str(), (int)dStr.length(), f60);
                    int maxW = (std::max)(nw, dw);
                    int cx = x + 110;

                    DrawFormatStringToHandle(cx - nw / 2 + 4, scoreY + 16, COL_TEXT_DARK(), f60, "%s", nStr.c_str());
                    DrawFormatStringToHandle(cx - nw / 2, scoreY + 12, fracCol, f60, "%s", nStr.c_str());
                    DrawLine(cx - maxW / 2 - 10, scoreY + 75, cx + maxW / 2 + 10, scoreY + 75, fracCol, 4);
                    DrawFormatStringToHandle(cx - dw / 2 + 4, scoreY + 84, COL_TEXT_DARK(), f60, "%s", dStr.c_str());
                    DrawFormatStringToHandle(cx - dw / 2, scoreY + 80, fracCol, f60, "%s", dStr.c_str());
                }

                // スコアプレビューUI
                if (p_hasScorePreview && !(p_previewScore.n == f.n && p_previewScore.d == f.d)) {
                    int f60 = GetCachedFont(60), f40 = GetCachedFont(40), f20 = GetCachedFont(20);
                    unsigned int previewCol = (p_previewScore.n == master.m_targetScore) ? COL_WARN() : (p_previewScore.n < f.n ? COL_DANGER() : COL_SAFE());
                    int arrowX = x + 240;

                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(150 + 100 * std::sin(GetNowCount() / 100.0)));
                    DrawStringToHandle(arrowX, scoreY + 45, "→", COL_TEXT_SUB(), f40);

                    if (p_previewScore.d == 1) {
                        std::string pStr = std::to_string(p_previewScore.n);
                        DrawStringToHandle(arrowX + 54, scoreY + 34, pStr.c_str(), COL_TEXT_DARK(), f60);
                        DrawStringToHandle(arrowX + 50, scoreY + 30, pStr.c_str(), previewCol, f60);
                    }
                    else {
                        std::string pnStr = std::to_string(p_previewScore.n), pdStr = std::to_string(p_previewScore.d);
                        int f30 = GetCachedFont(30);
                        DrawFormatStringToHandle(arrowX + 60, scoreY + 20, previewCol, f30, "%s", pnStr.c_str());
                        DrawLine(arrowX + 55, scoreY + 60, arrowX + 110, scoreY + 60, previewCol, 3);
                        DrawFormatStringToHandle(arrowX + 60, scoreY + 65, previewCol, f30, "%s", pdStr.c_str());
                    }
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                    if (p_previewScore.n > f.n && p_previewScore.n != master.m_targetScore) {
                        DrawFormatStringToHandle(arrowX + 50, scoreY + 100, COL_SAFE(), f20, "変動: %d 点", p_previewScore.n - f.n); // 「+」を削除しスマートに
                    }
                }

                // 目標スコア
                int targetBoxY = scoreY + 140;
                DrawCyberPanel(x + 10, targetBoxY, 480, 40, GetColor(20, 30, 40), COL_INFO(), 255);
                DrawStringToHandle(x + 20, targetBoxY + 10, "目標スコア", COL_INFO(), GetCachedFont(20));
                DrawFormatStringToHandle(x + 390, targetBoxY + 4, COL_TEXT_DARK(), GetCachedFont(32), "000");
                DrawFormatStringToHandle(x + 390, targetBoxY + 4, COL_SAFE(), GetCachedFont(32), "%03d", master.m_targetScore);
            }
            else {
                DrawCyberPanel(x + 10, scoreY, 480, 40, COL_DARK_BG(), COL_TEXT_SUB(), 255);
                DrawStringToHandle(x + 20, scoreY + 10, "ノーマルバトル", COL_TEXT_SUB(), GetCachedFont(20));
            }

            int powerY = scoreY + (master.m_ruleMode == BattleMaster::RuleMode::ZERO_ONE ? 190 : 50);
            DrawCyberPanel(x + 10, powerY, 480, 110, COL_DARK_BG(), baseCol, 255);
            DrawStringToHandle(x + 20, powerY + 10, "パワー", COL_WARN(), GetCachedFont(24));
            DrawLine(x + 100, powerY + 24, x + 480, powerY + 24, GetColor(60, 60, 70), 1);

            int currentNum = unit->GetNumber();
            int preview = (p_previewNum != -1) ? p_previewNum : currentNum;
            float cursorX = is1P ? m_uiCursorX_1P : m_uiCursorX_2P;
            if (cursorX == 0.0f) cursorX = 40.0f + (currentNum - 1) * 48.0f;

            DrawBox(x + 15, powerY + 40, x + 485, powerY + 90, GetColor(10, 10, 15), TRUE);
            DrawLine(x + 15, powerY + 65, x + 485, powerY + 65, GetColor(30, 30, 40), 1);

            bool isDeadPreview = (p_previewStocks < 0) && (master.m_ruleMode == BattleMaster::RuleMode::CLASSIC);
            unsigned int frameCol = isDeadPreview ? COL_DANGER() : baseCol;

            // アクティブ枠（サイバーブラケット）
            int hx = x + (int)cursorX, hy = powerY + 29, fw = 52, fh = 66;
            SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
            DrawBox(hx - fw / 2, hy, hx + fw / 2, hy + fh, frameCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int cl = 10, thick = 2;
            DrawBox(hx - fw / 2, hy, hx - fw / 2 + cl, hy + thick, frameCol, TRUE);
            DrawBox(hx - fw / 2, hy, hx - fw / 2 + thick, hy + cl, frameCol, TRUE);
            DrawBox(hx + fw / 2 - cl, hy, hx + fw / 2, hy + thick, frameCol, TRUE);
            DrawBox(hx + fw / 2 - thick, hy, hx + fw / 2, hy + cl, frameCol, TRUE);
            DrawBox(hx - fw / 2, hy + fh - thick, hx - fw / 2 + cl, hy + fh, frameCol, TRUE);
            DrawBox(hx - fw / 2, hy + fh - cl, hx - fw / 2 + thick, hy + fh, frameCol, TRUE);
            DrawBox(hx + fw / 2 - cl, hy + fh - thick, hx + fw / 2, hy + fh, frameCol, TRUE);
            DrawBox(hx + fw / 2 - thick, hy + fh - cl, hx + fw / 2, hy + fh, frameCol, TRUE);

            int f32Num = GetCachedFont(32), f16Tri = GetCachedFont(16);
            for (int i = 1; i <= 9; ++i) {
                int px = x + 40 + (i - 1) * 48, py = powerY + 43;
                std::string numStr = std::to_string(i);
                float targetX = 40.0f + (i - 1) * 48.0f;
                float distance = std::abs(cursorX - targetX);
                float currentYOff = 6.0f;
                unsigned int numCol = GetColor(70, 70, 80);

                if (distance < 48.0f && !isDeadPreview) {
                    float ratio = 1.0f - (distance / 48.0f);
                    ratio = std::sin(ratio * M_PI / 2.0f);
                    currentYOff = 6.0f - (20.0f * ratio);
                    numCol = (ratio > 0.8f) ? COL_TEXT_MAIN() : COL_TEXT_SUB();
                }

                if (i == preview && preview != currentNum && !isDeadPreview) {
                    currentYOff = -14.0f; numCol = COL_SAFE();
                    int twTriangle = GetDrawStringWidthToHandle("▲", 2, f16Tri);
                    DrawStringToHandle(px - twTriangle / 2, py + 26, "▲", COL_SAFE(), f16Tri);
                }

                int tw = GetDrawStringWidthToHandle(numStr.c_str(), 1, f32Num);
                DrawStringToHandle(px - tw / 2, py + (int)currentYOff, numStr.c_str(), numCol, f32Num);
            }

            int infoY = powerY + 120;
            DrawCyberPanel(x + 10, infoY, 480, 110, COL_DARK_BG(), baseCol, 255);
            int f22 = GetCachedFont(22);

            // バッテリー描画
            if (master.m_ruleMode == BattleMaster::RuleMode::CLASSIC) {
                DrawStringToHandle(x + 20, infoY + 15, "バッテリー", GetColor(180, 180, 180), f22);
                int currentStocks = unit->GetStocks();
                int previewStocks = (p_previewStocks != -1) ? p_previewStocks : currentStocks;
                // サイバーな斜めゲージを描画！
                DrawBatteryGauge(x + 140, infoY + 12, currentStocks, previewStocks, unit->GetMaxStocks(), COL_SAFE(), COL_DANGER(), baseCol);
            }

            int moveDist = 3 - ((unit->GetNumber() - 1) % 3);
            if (p_previewNum != -1 && p_previewNum != unit->GetNumber()) {
                int nextMoveDist = 3 - ((p_previewNum - 1) % 3);
                DrawFormatStringToHandle(x + 20, infoY + 65, COL_TEXT_SUB(), f22, "移動可能: %d → %d マス", moveDist, nextMoveDist);
            }
            else {
                DrawFormatStringToHandle(x + 20, infoY + 65, COL_TEXT_SUB(), f22, "移動可能: %d マス", moveDist);
            }

            // 移動範囲プレビュー（ドット）
            int ix = x + 350, iy = infoY + 15;
            DrawStringToHandle(ix - 5, iy, "【移動範囲】", COL_DISABLE(), GetCachedFont(18));
            int targetNumForGrid = (p_previewNum != -1) ? p_previewNum : unit->GetNumber();
            unsigned int gridBaseCol = (p_previewNum != -1) ? COL_SAFE() : baseCol;

            for (int i = 0; i < 9; ++i) {
                int gx = i % 3, gy = i / 3;
                bool canGo = (i == 4) ? false : (targetNumForGrid <= 3 ? (gx == 1 || gy == 1) : (targetNumForGrid <= 6 ? (gx == gy || gx + gy == 2) : true));
                unsigned int dotCol = canGo ? gridBaseCol : GetColor(40, 40, 50);
                DrawBox(ix + gx * 22, iy + 26 + gy * 22, ix + gx * 22 + 18, iy + 26 + gy * 22 + 18, dotCol, TRUE);
            }
            };

        drawUnitCard(40, 100, master.m_player.get(), true, p1PreviewNum, p1PreviewStocks, p1PreviewScore, p1HasScorePreview);
        drawUnitCard(1380, 100, master.m_enemy.get(), false, p2PreviewNum, p2PreviewStocks, p2PreviewScore, p2HasScorePreview);

        // ==========================================
        // 5. ログパネル
        // ==========================================
        int f22 = GetCachedFont(22), f20 = GetCachedFont(20);
        DrawCyberPanel(40, LOG_PANEL_Y, 500, 200, COL_PANEL_BG(), GetColor(100, 100, 120), 150);
        DrawStringToHandle(55, LOG_PANEL_Y + 10, "■ 戦況ログ", COL_DISABLE(), f22);
        DrawLine(50, LOG_PANEL_Y + 35, 530, LOG_PANEL_Y + 35, GetColor(60, 60, 70), 1);

        int maxLogOffset = std::max(0, (int)m_actionLog.size() - 6);
        if (maxLogOffset > 0) {
            DrawBox(520, LOG_PANEL_Y + 45, 525, LOG_PANEL_Y + 185, GetColor(30, 30, 40), TRUE);
            float scrollRatio = (float)m_logScrollOffset / maxLogOffset;
            int thumbY = LOG_PANEL_Y + 45 + (int)(scrollRatio * (140 - 30));
            DrawBox(520, thumbY, 525, thumbY + 30, GetColor(100, 150, 200), TRUE);
        }

        int startIdx = m_logScrollOffset, endIdx = std::min((int)m_actionLog.size(), startIdx + 6);
        for (int i = startIdx; i < endIdx; ++i) {
            int drawY = LOG_PANEL_Y + 45 + (i - startIdx) * 24;
            DrawFormatStringToHandle(55, drawY, GetColor(180, 220, 160), f20, "%s", m_actionLog[i].c_str());
        }

        // 基本ルールパネル
        DrawCyberPanel(1380, LOG_PANEL_Y, 500, 200, COL_PANEL_BG(), GetColor(100, 100, 120), 150);
        DrawStringToHandle(1395, LOG_PANEL_Y + 10, "■ 基本ルール", COL_DISABLE(), f22);
        DrawLine(1390, LOG_PANEL_Y + 35, 1870, LOG_PANEL_Y + 35, GetColor(60, 60, 70), 1);
        DrawStringToHandle(1395, LOG_PANEL_Y + 45, "パワー [1, 4, 7] : 3マス移動 (十字)", GetColor(255, 200, 100), f20);
        DrawStringToHandle(1395, LOG_PANEL_Y + 75, "パワー [2, 5, 8] : 2マス移動 (斜め)", GetColor(180, 180, 180), f20);
        DrawStringToHandle(1395, LOG_PANEL_Y + 105, "パワー [3, 6, 9] : 1マス移動 (全方位)", GetColor(100, 150, 255), f20);
        DrawStringToHandle(1395, LOG_PANEL_Y + 135, "演算子取得で攻撃・移動ルート拡張", GetColor(255, 255, 180), f20);
        DrawStringToHandle(1395, LOG_PANEL_Y + 165, "÷ は (自分,相手) の座標にワープを設置", COL_INFO(), f20);

        // ==========================================
        // 6. 下部パネル（計算式・プレビュー）
        // ==========================================
        unsigned int calcBorderCol = master.Is1PTurn() ? COL_P1() : COL_P2();
        DrawCyberPanel(600, BOTTOM_PANEL_Y, 720, 160, COL_BOTTOM_BG(), calcBorderCol, 220);

        if (showCalcPanel) {
            int f64 = GetCachedFont(64), f16 = GetCachedFont(16), f20 = GetCachedFont(20);

            if (isPreviewMode) {
                int preY = BOTTOM_PANEL_Y - 26;
                if (willGetNewOp) DrawFormatStringToHandle(600, preY, GetColor(255, 200, 100), f20, "▼ 移動プレビュー(演算子 [%c] を取得してバトル)", disp_aOp);
                else DrawStringToHandle(600, preY, "▼ 移動プレビュー(バトル発生)", GetColor(255, 200, 100), f20);
            }

            std::string leftLabel = (disp_aOp == '/') ? "X座標" : "自分";
            std::string rightLabel = (disp_aOp == '/') ? "Y座標" : "相手";
            unsigned int leftCol = (disp_aOp == '/') ? COL_INFO() : (master.Is1PTurn() ? COL_P1() : COL_P2());
            unsigned int rightCol = (disp_aOp == '/') ? COL_INFO() : (master.Is1PTurn() ? COL_P2() : COL_P1());

            int calcY = BOTTOM_PANEL_Y + 22;
            DrawStringToHandle(662, calcY - 18, leftLabel.c_str(), leftCol, f16);
            DrawStringToHandle(750, calcY - 18, rightLabel.c_str(), rightCol, f16);

            DrawFormatStringToHandle(662, calcY, COL_TEXT_DARK(), f64, "%d %c %d =", disp_aNum, disp_aOp, disp_tNum);
            DrawFormatStringToHandle(660, calcY - 2, COL_TEXT_MAIN(), f64, "%d %c %d =", disp_aNum, disp_aOp, disp_tNum);

            unsigned int resColor = (intRes < 0) ? COL_DANGER() : (!isCleanDivide ? COL_DISABLE() : (master.m_ruleMode == BattleMaster::RuleMode::ZERO_ONE ? COL_WARN() : COL_SAFE()));
            int resX = (master.m_ruleMode == BattleMaster::RuleMode::ZERO_ONE) ? 1000 : 1020;

            if (master.m_ruleMode == BattleMaster::RuleMode::CLASSIC || resFrac.d == 1 || !isCleanDivide) {
                // ★修正：「+」を表示せず、そのまま数字を描画する！
                DrawFormatStringToHandle(resX + 2, calcY, COL_TEXT_DARK(), f64, "%d", intRes);
                DrawFormatStringToHandle(resX, calcY - 2, resColor, f64, "%d", intRes);
            }

            if (master.m_ruleMode == BattleMaster::RuleMode::ZERO_ONE && isPreviewMode) {
                Fraction goal(master.m_targetScore);
                Fraction nextMy = (master.Is1PTurn() ? master.m_p1ZeroOneScore : master.m_p2ZeroOneScore) + resFrac;
                if (nextMy > goal) nextMy = goal - (nextMy - goal);
                Fraction nextEn = (master.Is1PTurn() ? master.m_p2ZeroOneScore : master.m_p1ZeroOneScore) + resFrac;
                if (nextEn > goal) nextEn = goal - (nextEn - goal);

                std::string strMy = (nextMy.d == 1) ? std::to_string(nextMy.n) : (std::to_string(nextMy.n) + "/" + std::to_string(nextMy.d));
                std::string strEn = (nextEn.d == 1) ? std::to_string(nextEn.n) : (std::to_string(nextEn.n) + "/" + std::to_string(nextEn.d));

                DrawStringToHandle(630, BOTTOM_PANEL_Y + 95, "【自分に反映】", COL_SAFE(), f22);
                DrawFormatStringToHandle(770, BOTTOM_PANEL_Y + 95, COL_TEXT_SUB(), f22, "スコア: %s", strMy.c_str());
                DrawStringToHandle(970, BOTTOM_PANEL_Y + 95, "【相手に反映】", COL_DANGER(), f22);
                DrawFormatStringToHandle(1110, BOTTOM_PANEL_Y + 95, COL_TEXT_SUB(), f22, "スコア: %s", strEn.c_str());

                if (disp_aOp == '/') {
                    int wx = disp_aNum - 1, wy = 9 - disp_tNum;
                    if (isCleanDivide) {
                        DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 125, COL_SAFE(), f20, "座標(%d, %d)に【自分】のワープ設置！", wx, wy);
                        DrawFormatStringToHandle(970, BOTTOM_PANEL_Y + 125, COL_DANGER(), f20, "座標(%d, %d)に【相手】のワープ設置！", wx, wy);
                    }
                    else {
                        DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 125, COL_INFO(), f20, "座標(%d, %d)にワープ設置 (スコア変動なし)", wx, wy);
                    }
                }
            }
            else if (master.m_ruleMode == BattleMaster::RuleMode::CLASSIC && isPreviewMode) {
                if (disp_aOp == '/' && !isCleanDivide) {
                    DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 95, COL_INFO(), f22, "【ワープ設置】 座標(%d, %d) にワープを設置します", disp_aNum, disp_tNum);
                }
                else {
                    auto calcDmg = [](int currentHp, int currentStocks, int val, int& outHp, int& outStocks) {
                        outHp = currentHp + val; outStocks = currentStocks;
                        while (outHp <= 0) { outStocks--; outHp += 9; }
                        while (outHp > 9) { outStocks++; outHp -= 9; }
                        };
                    int myHp, mySt, enHp, enSt;
                    calcDmg(disp_aNum, activeActor->GetStocks(), intRes, myHp, mySt);
                    calcDmg(disp_tNum, activeTarget->GetStocks(), intRes, enHp, enSt);

                    DrawStringToHandle(630, BOTTOM_PANEL_Y + 95, "【自分に反映】", COL_SAFE(), f22);
                    if (mySt <= 0 && myHp <= 0) DrawStringToHandle(770, BOTTOM_PANEL_Y + 95, "敗北", COL_DANGER(), f22);
                    else DrawFormatStringToHandle(770, BOTTOM_PANEL_Y + 95, COL_TEXT_SUB(), f22, "残機 %d | パワー %d", mySt, myHp);

                    DrawStringToHandle(970, BOTTOM_PANEL_Y + 95, "【相手に反映】", COL_DANGER(), f22);
                    if (enSt <= 0 && enHp <= 0) DrawStringToHandle(1110, BOTTOM_PANEL_Y + 95, "撃破！", COL_WARN(), f22);
                    else DrawFormatStringToHandle(1110, BOTTOM_PANEL_Y + 95, COL_TEXT_SUB(), f22, "残機 %d | パワー %d", enSt, enHp);

                    if (disp_aOp == '/') {
                        DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 125, COL_SAFE(), f20, "座標(%d, %d)に【自分】のワープ設置！", disp_aNum, disp_tNum);
                        DrawFormatStringToHandle(970, BOTTOM_PANEL_Y + 125, COL_DANGER(), f20, "座標(%d, %d)に【相手】のワープ設置！", disp_aNum, disp_tNum);
                    }
                }
            }
            else if (!isPreviewMode) {
                if (isHoverSelf || isHoverEnemy) {
                    std::string targetName = isHoverSelf ? (activeActor == master.m_player.get() ? "1P" : "2P") : (activeTarget == master.m_player.get() ? "1P" : "2P");
                    if (master.m_ruleMode == BattleMaster::RuleMode::CLASSIC) {
                        int simulatedHp = intRes, stockChange = 0;
                        while (simulatedHp <= 0) { stockChange--; simulatedHp += 9; }
                        while (simulatedHp > 9) { stockChange++; simulatedHp -= 9; }

                        if (stockChange < 0) DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 92, COL_DANGER(), f22, "▼ 攻撃！ %s の【バッテリー %d】、パワー [%d] に", targetName.c_str(), stockChange, simulatedHp);
                        else if (stockChange > 0) DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 92, COL_SAFE(), f22, "▼ 回復！ %s の【バッテリー +%d】、パワー [%d] に", targetName.c_str(), stockChange, simulatedHp);
                        else DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 92, COL_TEXT_SUB(), f22, "▼ 反映: %s のパワーを [%d] にします", targetName.c_str(), simulatedHp);
                    }
                    else {
                        DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 92, COL_TEXT_MAIN(), f22, "▼ %s のスコアにこの結果を反映", targetName.c_str());
                    }

                    if (disp_aOp == '/') {
                        unsigned int warpCol = isHoverSelf ? COL_SAFE() : COL_DANGER();
                        if (isCleanDivide) DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 122, warpCol, f20, "さらに座標(%d, %d)に【%s】のワープ設置！", disp_aNum, disp_tNum, targetName.c_str());
                        else DrawFormatStringToHandle(630, BOTTOM_PANEL_Y + 122, warpCol, f20, "座標(%d, %d)に【%s】のワープ設置！", disp_aNum, disp_tNum, targetName.c_str());
                    }
                }
                else {
                    DrawStringToHandle(730, BOTTOM_PANEL_Y + 92, "反映する対象を選択してください", GetColor(120, 120, 130), f22);
                    if (disp_aOp == '/') DrawFormatStringToHandle(730, BOTTOM_PANEL_Y + 122, COL_INFO(), f20, "実行時、選択した対象にワープを設置！");
                }
            }
        }
        else if (master.m_currentPhase == BattleMaster::Phase::P1_Action || master.m_currentPhase == BattleMaster::Phase::P2_Action) {
            int f28 = GetCachedFont(28);
            if (canAttack && !hasOp) DrawStringToHandle(730, BOTTOM_PANEL_Y + 60, "【 演算子アイテムが必要です 】", COL_DANGER(), f28);
            else DrawStringToHandle(770, BOTTOM_PANEL_Y + 60, "ターゲットが射程内にいません", COL_DISABLE(), f28);
        }
        else {
            DrawStringToHandle(750, BOTTOM_PANEL_Y + 60, "移動するマスを選択してください", COL_DISABLE(), GetCachedFont(28));
        }

        // ==========================================
        // 7. 決定ボタン描画（行動フェーズ）
        // ==========================================
        if (master.m_currentPhase == BattleMaster::Phase::P1_Action || master.m_currentPhase == BattleMaster::Phase::P2_Action) {
            if (!((master.m_currentPhase == BattleMaster::Phase::P1_Action && master.m_is1P_NPC) ||
                (master.m_currentPhase == BattleMaster::Phase::P2_Action && master.m_is2P_NPC))) {

                int by = 960, f26 = GetCachedFont(26);
                if (canAttack && hasOp) {
                    DrawCyberButton(600, by, 220, 60, "自分", COL_SAFE(), isHoverSelf, f26);
                    DrawCyberButton(850, by, 220, 60, "相手", COL_DANGER(), isHoverEnemy, f26);
                    DrawCyberButton(1100, by, 220, 60, "何もしない", COL_DISABLE(), master.CheckButtonClick(1100, by, 220, 60, mousePos), f26);
                }
                else {
                    bool hover = master.CheckButtonClick(750, by, 420, 60, mousePos);
                    DrawCyberButton(750, by, 420, 60, "ターン終了", GetColor(180, 180, 200), hover, f26);
                }
            }
        }

        // ==========================================
        // ターン開始カットイン
        // ==========================================
        if (!master.IsGameOver() && (master.m_currentPhase == BattleMaster::Phase::P1_TurnStart || master.m_currentPhase == BattleMaster::Phase::P2_TurnStart)) {
            bool is1P = (master.m_currentPhase == BattleMaster::Phase::P1_TurnStart);
            int timer = master.GetTurnStartTimer();

            int darkAlpha = (timer > 60) ? (80 - timer) * 7 : ((timer < 20) ? timer * 7 : 140);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, darkAlpha);
            DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(0, 0, 0), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int bandH = 160;
            unsigned int bandCol = is1P ? COL_P1() : COL_P2();
            float scale = 1.0f;
            if (timer > 70) scale = (80 - timer) / 10.0f;
            else if (timer < 10) scale = std::max(0.0f, timer / 10.0f);

            int currentH = (int)(bandH * scale);
            int currentY = SCREEN_H / 2 - currentH / 2;

            if (currentH > 0) {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
                DrawBox(0, currentY, SCREEN_W, currentY + currentH, GetColor(15, 20, 25), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                DrawLine(0, currentY, SCREEN_W, currentY, bandCol, 3);
                DrawLine(0, currentY + currentH, SCREEN_W, currentY + currentH, bandCol, 3);
            }

            if (timer <= 75 && timer >= 5) {
                int f80 = GetCachedFont(80);
                std::string text = is1P ? (master.m_is1P_NPC ? "1P (COM) TURN" : "1P TURN") : (master.m_is2P_NPC ? "2P (COM) TURN" : "2P TURN");
                int tw = GetDrawStringWidthToHandle(text.c_str(), (int)text.length(), f80);
                int textX = SCREEN_W / 2 - tw / 2 + (40 - timer);
                int textY = SCREEN_H / 2 - 40;

                SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
                DrawStringToHandle(textX - 4, textY, text.c_str(), bandCol, f80);
                DrawStringToHandle(textX + 4, textY, text.c_str(), bandCol, f80);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                DrawStringToHandle(textX, textY, text.c_str(), COL_TEXT_MAIN(), f80);
            }
        }

        // ==========================================
        // ポーズボタン & 終了画面
        // ==========================================
        int pauseBtnX = SCREEN_W - 180, pauseBtnY = 10, pauseBtnW = 160, pauseBtnH = 50;
        DrawCyberButton(pauseBtnX, pauseBtnY, pauseBtnW, pauseBtnH, "ポーズ (ESC)", COL_WARN(), master.CheckButtonClick(pauseBtnX, pauseBtnY, pauseBtnW, pauseBtnH, mousePos), GetCachedFont(22));

        if (master.m_currentPhase == BattleMaster::Phase::FINISH) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
            DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_TEXT_DARK(), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int f160 = GetCachedFont(160);
            const char* finishText = "ゲームセット";
            int textW = GetDrawStringWidthToHandle(finishText, (int)strlen(finishText), f160);
            DrawStringToHandle(SCREEN_W / 2 - textW / 2, SCREEN_H / 2 - 100, finishText, COL_TEXT_MAIN(), f160);

            if (master.m_finishTimer > 30) {
                int f40 = GetCachedFont(40);
                const char* nextText = ">> クリックで進む <<";
                int nw = GetDrawStringWidthToHandle(nextText, (int)strlen(nextText), f40);
                DrawStringToHandle(SCREEN_W / 2 - nw / 2, SCREEN_H / 2 + 100, nextText, COL_TEXT_SUB(), f40);
            }
        }

        m_commUI.Draw();
    }

    // ==========================================
        // チュートリアル用のUI描画（カウント・ノーマル両対応版）
        // ==========================================
    void BattleUI::Draw(const TutorialMaster& master) const {
        if (m_psHandle != -1 && m_cbHandle != -1) {
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            cb[0] = m_shaderTime; cb[1] = SCREEN_W; cb[2] = SCREEN_H; cb[3] = 0.0f;
            UpdateShaderConstantBuffer(m_cbHandle);
            SetShaderConstantBuffer(m_cbHandle, DX_SHADERTYPE_PIXEL, 0);

            SetUsePixelShader(m_psHandle);
            VERTEX2DSHADER v[6];
            for (int i = 0; i < 6; ++i) {
                v[i].pos = VGet(0, 0, 0); v[i].rhw = 1.0f;
                v[i].dif = GetColorU8(255, 255, 255, 255); v[i].spc = GetColorU8(0, 0, 0, 0);
                v[i].u = 0.0f; v[i].v = 0.0f;
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

        DrawCyberPanel(0, HEADER_H, 580, SCREEN_H - HEADER_H, COL_PANEL_BG(), COL_P1(), 180);
        DrawCyberPanel(1340, HEADER_H, SCREEN_W - 1340, SCREEN_H - HEADER_H, COL_PANEL_BG(), COL_P2(), 180);

        master.m_mapGrid.Draw();
        if (master.m_player) master.m_player->Draw();
        if (master.m_enemy)  master.m_enemy->Draw();

        DrawBox(0, 0, SCREEN_W, HEADER_H, COL_INFO(), TRUE);
        DrawLine(0, HEADER_H, SCREEN_W, HEADER_H, COL_TEXT_MAIN(), 2);
        DrawFormatStringToHandle(40, 16, COL_TEXT_MAIN(), GetCachedFont(38), ">>> チュートリアル");

        // ==========================================
        // ★修正：ルールに応じてUIを切り替えるように変更
        // ==========================================
        auto drawSimpleUnitCard = [&](int x, int y, UnitBase* unit, bool is1P) {
            if (!unit) return;
            unsigned int baseCol = is1P ? COL_P1() : COL_P2();
            std::string headerName = is1P ? "1P (YOU)" : "2P (ENEMY)";

            DrawCyberPanel(x, y, 500, 490, GetColor(15, 18, 25), baseCol, 220);
            DrawStringToHandle(x + 15, y + 10, headerName.c_str(), baseCol, GetCachedFont(36));
            DrawLine(x + 10, y + 50, x + 490, y + 50, baseCol, 2);

            int scoreY = y + 60;

            // カウントバトルモードのUI
            if (master.m_ruleMode == 1) {
                DrawCyberPanel(x + 10, scoreY, 480, 130, COL_DARK_BG(), baseCol, 255);
                DrawStringToHandle(x + 20, scoreY + 5, "現在のスコア", COL_TEXT_SUB(), GetCachedFont(18));

                int displayScore = is1P ? (int)master.m_p1DisplayScore : (int)master.m_p2DisplayScore;
                int f100 = GetCachedFont(100);
                std::string nStr = std::to_string(displayScore);
                DrawStringToHandle(x + 34, scoreY + 24, nStr.c_str(), COL_TEXT_DARK(), f100);
                DrawStringToHandle(x + 30, scoreY + 20, nStr.c_str(), COL_TEXT_MAIN(), f100);

                int targetBoxY = scoreY + 140;
                DrawCyberPanel(x + 10, targetBoxY, 480, 40, GetColor(20, 30, 40), COL_INFO(), 255);
                DrawStringToHandle(x + 20, targetBoxY + 10, "目標スコア", COL_INFO(), GetCachedFont(20));
                DrawFormatStringToHandle(x + 390, targetBoxY + 4, COL_TEXT_DARK(), GetCachedFont(32), "000");
                DrawFormatStringToHandle(x + 390, targetBoxY + 4, COL_SAFE(), GetCachedFont(32), "%03d", master.m_targetScore);
            }
            // ノーマルバトルモードのUI
            else {
                DrawCyberPanel(x + 10, scoreY, 480, 40, COL_DARK_BG(), COL_TEXT_SUB(), 255);
                DrawStringToHandle(x + 20, scoreY + 10, "ノーマルバトル", COL_TEXT_SUB(), GetCachedFont(20));
            }

            int powerY = scoreY + (master.m_ruleMode == 1 ? 190 : 50);
            DrawCyberPanel(x + 10, powerY, 480, 110, COL_DARK_BG(), baseCol, 255);
            DrawStringToHandle(x + 20, powerY + 10, "パワー", COL_WARN(), GetCachedFont(24));
            DrawLine(x + 100, powerY + 24, x + 480, powerY + 24, GetColor(60, 60, 70), 1);

            DrawBox(x + 15, powerY + 40, x + 485, powerY + 90, GetColor(10, 10, 15), TRUE);
            DrawLine(x + 15, powerY + 65, x + 485, powerY + 65, GetColor(30, 30, 40), 1);

            int currentNum = unit->GetNumber(), f32Num = GetCachedFont(32);
            for (int i = 1; i <= 9; ++i) {
                int px = x + 40 + (i - 1) * 48, py = powerY + 43;
                unsigned int numCol = (i == currentNum) ? COL_SAFE() : GetColor(70, 70, 80);
                std::string numStr = std::to_string(i);
                int tw = GetDrawStringWidthToHandle(numStr.c_str(), 1, f32Num);
                DrawStringToHandle(px - tw / 2, py + ((i == currentNum) ? -10 : 6), numStr.c_str(), numCol, f32Num);
            }

            int infoY = powerY + 120;
            DrawCyberPanel(x + 10, infoY, 480, 110, COL_DARK_BG(), baseCol, 255);

            // ノーマルバトルの時だけバッテリーを描画
            if (master.m_ruleMode == 0) {
                DrawStringToHandle(x + 20, infoY + 15, "バッテリー", GetColor(180, 180, 180), GetCachedFont(22));
                DrawBatteryGauge(x + 140, infoY + 12, unit->GetStocks(), unit->GetStocks(), unit->GetMaxStocks(), COL_SAFE(), COL_DANGER(), baseCol);
            }

            int moveDist = 3 - ((unit->GetNumber() - 1) % 3);
            DrawFormatStringToHandle(x + 20, infoY + 65, COL_TEXT_SUB(), GetCachedFont(22), "移動可能: %d マス", moveDist);

            int ix = x + 350, iy = infoY + 15;
            DrawStringToHandle(ix - 5, iy, "【移動範囲】", COL_DISABLE(), GetCachedFont(18));
            int targetNumForGrid = unit->GetNumber();
            for (int i = 0; i < 9; ++i) {
                int gx = i % 3, gy = i / 3;
                bool canGo = (i == 4) ? false : (targetNumForGrid <= 3 ? (gx == 1 || gy == 1) : (targetNumForGrid <= 6 ? (gx == gy || gx + gy == 2) : true));
                unsigned int dotCol = canGo ? baseCol : GetColor(40, 40, 50);
                DrawBox(ix + gx * 22, iy + 26 + gy * 22, ix + gx * 22 + 18, iy + 26 + gy * 22 + 18, dotCol, TRUE);
            }
            };

        drawSimpleUnitCard(40, 100, master.m_player.get(), true);
        drawSimpleUnitCard(1380, 100, master.m_enemy.get(), false);

        int f22 = GetCachedFont(22), f20 = GetCachedFont(20);
        DrawCyberPanel(40, LOG_PANEL_Y, 500, 200, COL_PANEL_BG(), GetColor(100, 100, 120), 150);
        DrawStringToHandle(55, LOG_PANEL_Y + 10, "■ 戦況ログ", COL_DISABLE(), f22);
        DrawLine(50, LOG_PANEL_Y + 35, 530, LOG_PANEL_Y + 35, GetColor(60, 60, 70), 1);

        int maxLogOffset = std::max(0, (int)m_actionLog.size() - 6);
        if (maxLogOffset > 0) {
            DrawBox(520, LOG_PANEL_Y + 45, 525, LOG_PANEL_Y + 185, GetColor(30, 30, 40), TRUE);
            float scrollRatio = (float)m_logScrollOffset / maxLogOffset;
            int thumbY = LOG_PANEL_Y + 45 + (int)(scrollRatio * (140 - 30));
            DrawBox(520, thumbY, 525, thumbY + 30, GetColor(100, 150, 200), TRUE);
        }
        int startIdx = m_logScrollOffset, endIdx = std::min((int)m_actionLog.size(), startIdx + 6);
        for (int i = startIdx; i < endIdx; ++i) {
            int drawY = LOG_PANEL_Y + 45 + (i - startIdx) * 24;
            DrawFormatStringToHandle(55, drawY, GetColor(180, 220, 160), f20, "%s", m_actionLog[i].c_str());
        }

        DrawCyberPanel(600, BOTTOM_PANEL_Y, 720, 160, COL_BOTTOM_BG(), COL_INFO(), 220);
        DrawStringToHandle(620, BOTTOM_PANEL_Y + 60, "中央のメッセージウィンドウの指示に従って操作してください。", COL_TEXT_MAIN(), f22);
    }
} // namespace App
