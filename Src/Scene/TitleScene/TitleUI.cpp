#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <DxLib.h>

#include "TitleUI.h"
#include "TitleUILayout.h"
#include "../../Input/InputManager.h"
#include "../../Manager/ProceduralAudio.h"
#include "../../Shader/CyberGrid.h"
#include "../../Shader/CrystalOrbShader.h"
#include "../../Shader/ImpactEffectShader.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    constexpr double BLINK_SPEED = 3.0;
    constexpr int BLINK_BASE_ALPHA = 150;
    constexpr int BLINK_AMP_ALPHA = 105;

    inline unsigned int COL_BG() { return GetColor(5, 10, 25); }
    inline unsigned int COL_GRID() { return GetColor(0, 150, 255); }
    inline unsigned int COL_P1() { return GetColor(255, 140, 0); }
    inline unsigned int COL_P2() { return GetColor(0, 150, 255); }
    inline unsigned int COL_TEXT_ON() { return GetColor(255, 180, 0); }
    inline unsigned int COL_TEXT_SUB() { return GetColor(200, 200, 200); }
    inline unsigned int COL_TEXT_DIM() { return GetColor(100, 100, 100); }
    inline unsigned int COL_TEXT_INFO() { return GetColor(200, 100, 255); }
    inline unsigned int COL_TEXT_WARN() { return GetColor(255, 255, 0); }
    inline unsigned int COL_TEXT_ERROR() { return GetColor(255, 100, 100); }
    inline unsigned int COL_TEXT_OFF() { return GetColor(50, 100, 150); }
    inline unsigned int COL_WHITE() { return GetColor(255, 255, 255); }
    inline unsigned int COL_BLACK() { return GetColor(0, 0, 0); }
    inline unsigned int COL_TITLE_MAIN() { return GetColor(220, 245, 255); }
    inline unsigned int COL_TITLE_SUB() { return GetColor(0, 120, 255); }
    inline unsigned int COL_DANGER() { return GetColor(255, 100, 100); }
}

namespace App {
    using namespace TitleLayout;

    TitleUI::TitleUI() = default;

    TitleUI::~TitleUI() = default;

    void TitleUI::Init() {
        m_fontTitle = CreateFontToHandle("BIZ UD明朝 Medium", 100, 3, DX_FONTTYPE_ANTIALIASING);
        m_fontMenu = CreateFontToHandle("BIZ UDゴシック", 40, 2, DX_FONTTYPE_NORMAL);
        m_fontSmall = CreateFontToHandle("遊ゴシック", 24, 2, DX_FONTTYPE_NORMAL);
        m_fontNumber = CreateFontToHandle("HGP創英角ﾎﾟｯﾌﾟ体", 48, 2, DX_FONTTYPE_ANTIALIASING);

        m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));
        m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        m_psCrystalHandle = LoadPixelShaderFromMem(g_ps_CrystalOrb, sizeof(g_ps_CrystalOrb));
        m_cbCrystalHandle = CreateShaderConstantBuffer(sizeof(float) * 8);
        m_psImpactHandle = LoadPixelShaderFromMem(g_ps_ImpactEffect, sizeof(g_ps_ImpactEffect));
        m_cbImpactHandle = CreateShaderConstantBuffer(sizeof(float) * 8);

        m_shaderTime = 0.0f;
        m_impactType = 2;
        m_miniGame.NextStage(false);
        m_ripples.clear();

        m_bouncingOps.clear();
        const std::string symbols[4] = { "+", "-", "*", "/" };
        const unsigned int opColors[4] = {
            GetColor(255, 100, 100),
            GetColor(100, 255, 100),
            GetColor(100, 200, 255),
            GetColor(255, 220, 50)
        };

        for (int i = 0; i < 4; ++i) {
            BouncingOp op;
            op.x = 200.0f + (GetNowCount() + i * 100) % 600;
            op.y = 150.0f + (GetNowCount() + i * 50) % 300;
            const float speedX = 2.5f + (i * 0.5f);
            const float speedY = 3.0f - (i * 0.3f);
            op.vx = (i % 2 == 0 ? speedX : -speedX);
            op.vy = (i < 2 ? speedY : -speedY);
            op.angle = 0.0f;
            op.symbol = symbols[i];
            op.color = opColors[i];
            m_bouncingOps.push_back(op);
        }
    }

    void TitleUI::Update(bool updateWaitingMiniGame) {
        auto& input = InputManager::GetInstance();
        m_shaderTime += 0.0016f;

        Vector2 mouse = input.GetMousePos();
        static Vector2 prevMouse = mouse;
        const float mouseVx = mouse.x - prevMouse.x;
        const float mouseVy = mouse.y - prevMouse.y;
        prevMouse = mouse;

        static int prevTab = 0;
        const int currentTab = CheckHitKey(KEY_INPUT_TAB);
        if (currentTab == 1 && prevTab == 0) {
            m_impactType = (m_impactType + 1) % 3;
            ProceduralAudio::GetInstance().PlayPowerSE(5);
        }
        prevTab = currentTab;

        int sw = 0, sh = 0;
        GetDrawScreenSize(&sw, &sh);
        constexpr float radius = 34.0f;
        constexpr float diameter = radius * 2.0f;

        static int grabbedIdx = -1;
        const bool isMouseHeld = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
        static bool prevMouseHeld = false;
        const bool justClicked = (isMouseHeld && !prevMouseHeld);
        prevMouseHeld = isMouseHeld;

        if (justClicked) {
            for (int i = static_cast<int>(m_bouncingOps.size()) - 1; i >= 0; --i) {
                const float dx = mouse.x - m_bouncingOps[i].x;
                const float dy = mouse.y - m_bouncingOps[i].y;
                if (dx * dx + dy * dy < radius * radius) {
                    grabbedIdx = i;
                    ProceduralAudio::GetInstance().PlayPowerSE(2);
                    break;
                }
            }
        }

        if (grabbedIdx != -1) {
            if (isMouseHeld) {
                m_bouncingOps[grabbedIdx].x = mouse.x;
                m_bouncingOps[grabbedIdx].y = mouse.y;
                m_bouncingOps[grabbedIdx].vx = mouseVx * 1.5f;
                m_bouncingOps[grabbedIdx].vy = mouseVy * 1.5f;
            }
            else {
                grabbedIdx = -1;
            }
        }

        for (size_t i = 0; i < m_bouncingOps.size(); ++i) {
            auto& op = m_bouncingOps[i];
            if (static_cast<int>(i) != grabbedIdx) {
                op.x += op.vx;
                op.y += op.vy;
                op.angle += op.vx * 0.02f + op.vy * 0.015f;
            }
        }

        for (size_t i = 0; i < m_bouncingOps.size(); ++i) {
            for (size_t j = i + 1; j < m_bouncingOps.size(); ++j) {
                auto& opA = m_bouncingOps[i];
                auto& opB = m_bouncingOps[j];
                const float dx = opB.x - opA.x;
                const float dy = opB.y - opA.y;
                const float distance = std::sqrt(dx * dx + dy * dy);

                if (distance < diameter) {
                    const float overlap = diameter - distance + 0.1f;
                    const float nx = dx / (distance == 0.0f ? 1.0f : distance);
                    const float ny = dy / (distance == 0.0f ? 1.0f : distance);

                    if (static_cast<int>(i) == grabbedIdx) {
                        opB.x += nx * overlap;
                        opB.y += ny * overlap;
                    }
                    else if (static_cast<int>(j) == grabbedIdx) {
                        opA.x -= nx * overlap;
                        opA.y -= ny * overlap;
                    }
                    else {
                        opA.x -= nx * (overlap * 0.5f);
                        opA.y -= ny * (overlap * 0.5f);
                        opB.x += nx * (overlap * 0.5f);
                        opB.y += ny * (overlap * 0.5f);
                    }

                    const float kx = opA.vx - opB.vx;
                    const float ky = opA.vy - opB.vy;
                    const float vn = nx * kx + ny * ky;

                    if (vn > 0.0f) {
                        constexpr float e = 1.0f;
                        if (static_cast<int>(i) == grabbedIdx) {
                            const float impulse = (1.0f + e) * vn;
                            opB.vx += nx * impulse;
                            opB.vy += ny * impulse;
                        }
                        else if (static_cast<int>(j) == grabbedIdx) {
                            const float impulse = (1.0f + e) * vn;
                            opA.vx -= nx * impulse;
                            opA.vy -= ny * impulse;
                        }
                        else {
                            const float impulse = (1.0f + e) * vn * 0.5f;
                            opA.vx -= nx * impulse;
                            opA.vy -= ny * impulse;
                            opB.vx += nx * impulse;
                            opB.vy += ny * impulse;
                        }
                        ProceduralAudio::GetInstance().PlayPowerSE(2);
                    }
                }
            }
        }

        constexpr float maxSpeed = 30.0f;
        for (size_t i = 0; i < m_bouncingOps.size(); ++i) {
            auto& op = m_bouncingOps[i];
            if (static_cast<int>(i) != grabbedIdx) {
                const float speedSq = op.vx * op.vx + op.vy * op.vy;
                if (speedSq > maxSpeed * maxSpeed) {
                    const float ratio = maxSpeed / std::sqrt(speedSq);
                    op.vx *= ratio;
                    op.vy *= ratio;
                }
            }

            bool hitWall = false;
            float hitX = op.x;
            float hitY = op.y;

            if (op.x < radius) {
                op.x = radius;
                if (static_cast<int>(i) != grabbedIdx) op.vx *= -1.0f;
                hitWall = true;
                hitX = 0.0f;
            }
            else if (op.x > sw - radius) {
                op.x = sw - radius;
                if (static_cast<int>(i) != grabbedIdx) op.vx *= -1.0f;
                hitWall = true;
                hitX = static_cast<float>(sw);
            }

            if (op.y < radius) {
                op.y = radius;
                if (static_cast<int>(i) != grabbedIdx) op.vy *= -1.0f;
                hitWall = true;
                hitY = 0.0f;
            }
            else if (op.y > sh - radius) {
                op.y = sh - radius;
                if (static_cast<int>(i) != grabbedIdx) op.vy *= -1.0f;
                hitWall = true;
                hitY = static_cast<float>(sh);
            }

            if (hitWall) {
                m_ripples.push_back({ hitX, hitY, 10.0f, 220.0f, op.color });
                ProceduralAudio::GetInstance().PlayPowerSE(2);
            }
        }

        if (input.IsMouseLeftTrg()) {
            m_ripples.push_back({ mouse.x, mouse.y, 0.0f, 200.0f, COL_TEXT_ON() });
        }

        for (auto& r : m_ripples) {
            r.radius += 12.0f;
            r.alpha -= 6.0f;
        }
        m_ripples.erase(
            std::remove_if(m_ripples.begin(), m_ripples.end(), [](const RippleEffect& r) { return r.alpha <= 0.0f; }),
            m_ripples.end());

        if (updateWaitingMiniGame) {
            m_miniGame.Update();
        }
    }

    void TitleUI::Draw(const TitleViewData& view) {
        if (view.setup == nullptr) return;

        const TitleState m_titleState = view.titleState;
        const NetSetupStep m_netStep = view.netStep;
        const BattleSetup::Step m_setupStep = view.setup->step;
        const int m_mainMenuCursor = view.mainMenuCursor;
        const int m_exitCursor = view.exitCursor;
        const int m_netRoleCursor = view.netRoleCursor;
        const int m_hostListCursor = view.hostListCursor;
        const int m_playerCursor = view.setup->playerCursor;
        const int m_modeCursor = view.setup->modeCursor;
        const int m_stocksCursor = view.setup->stocksCursor;
        const int m_scoreCursor = view.setup->scoreCursor;
        const int m_stageCursor = view.setup->stageCursor;
        const auto& m_players = view.setup->players;
        constexpr int GRID_SIZE = BattleSetup::GRID_SIZE;

        int sw, sh;
        GetDrawScreenSize(&sw, &sh);
        int CX = sw / 2;
        int CY = sh / 2;

        auto& input = InputManager::GetInstance();
        Vector2 m = input.GetMousePos();

        auto HoverBox = [&](int x, int y, int w, int h) {
            return (m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h);
            };

        // 背景描画
        DrawBox(0, 0, sw, sh, COL_BG(), TRUE);

        if (m_psHandle != -1 && m_cbHandle != -1) {
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            if (cb != nullptr) {
                cb[0] = m_shaderTime; cb[1] = (float)sw; cb[2] = (float)sh; cb[3] = 0.0f;
                UpdateShaderConstantBuffer(m_cbHandle);
                SetShaderConstantBuffer(m_cbHandle, DX_SHADERTYPE_PIXEL, 0);

                SetUsePixelShader(m_psHandle);
                VERTEX2DSHADER v[6];
                for (int i = 0; i < 6; ++i) {
                    v[i].pos = VGet(0, 0, 0); v[i].rhw = 1.0f;
                    v[i].dif = GetColorU8(255, 255, 255, 255); v[i].spc = GetColorU8(0, 0, 0, 0);
                }
                v[0].pos.x = 0;  v[0].pos.y = 0;  v[0].u = 0.0f; v[0].v = 0.0f;
                v[1].pos.x = sw; v[1].pos.y = 0;  v[1].u = 1.0f; v[1].v = 0.0f;
                v[2].pos.x = 0;  v[2].pos.y = sh; v[2].u = 0.0f; v[2].v = 1.0f;
                v[3].pos.x = sw; v[3].pos.y = 0;  v[3].u = 1.0f; v[3].v = 0.0f;
                v[4].pos.x = sw; v[4].pos.y = sh; v[4].u = 1.0f; v[4].v = 1.0f;
                v[5].pos.x = 0;  v[5].pos.y = sh; v[5].u = 0.0f; v[5].v = 1.0f;
                DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
                SetUsePixelShader(-1);
            }
        }

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 30);
        for (int i = 0; i < sw; i += MAP_CELL_SIZE) DrawLine(i, 0, i, sh, COL_GRID(), 1);
        for (int j = 0; j < sh; j += MAP_CELL_SIZE) DrawLine(0, j, sw, j, COL_GRID(), 1);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawBox(0, 40, sw, 45, COL_P1(), TRUE);
        DrawBox(0, sh - 45, sw, sh - 40, COL_P1(), TRUE);

        if (m_psCrystalHandle != -1 && m_cbCrystalHandle != -1) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
            double time = GetNowCount() / 1000.0;

            for (const auto& bOp : m_bouncingOps) {
                float r = 0.0f, g = 0.0f, b = 0.0f;
                if (bOp.symbol == "+") { r = 0.85f; g = 0.10f; b = 0.20f; }
                else if (bOp.symbol == "-") { r = 0.10f; g = 0.45f; b = 0.95f; }
                else if (bOp.symbol == "*") { r = 0.15f; g = 0.80f; b = 0.25f; }
                else { r = 0.70f; g = 0.15f; b = 0.90f; }

                SetUsePixelShader(m_psCrystalHandle);
                float* cb = (float*)GetBufferShaderConstantBuffer(m_cbCrystalHandle);
                cb[0] = (float)time; cb[1] = r; cb[2] = g; cb[3] = b;
                cb[4] = bOp.angle; cb[5] = 0.0f; cb[6] = 0.0f; cb[7] = 0.0f;
                UpdateShaderConstantBuffer(m_cbCrystalHandle);
                SetShaderConstantBuffer(m_cbCrystalHandle, DX_SHADERTYPE_PIXEL, 0);

                float size = 42.0f; float cx = bOp.x; float cy = bOp.y;
                VERTEX2DSHADER v[6];
                for (int i = 0; i < 6; ++i) {
                    v[i].pos = VGet(0, 0, 0); v[i].rhw = 1.0f;
                    v[i].dif = GetColorU8(255, 255, 255, 255); v[i].spc = GetColorU8(0, 0, 0, 0);
                }
                v[0].pos.x = cx - size; v[0].pos.y = cy - size; v[0].u = 0.0f; v[0].v = 0.0f;
                v[1].pos.x = cx + size; v[1].pos.y = cy - size; v[1].u = 1.0f; v[1].v = 0.0f;
                v[2].pos.x = cx - size; v[2].pos.y = cy + size; v[2].u = 0.0f; v[2].v = 1.0f;
                v[3].pos.x = cx + size; v[3].pos.y = cy - size; v[3].u = 1.0f; v[3].v = 0.0f;
                v[4].pos.x = cx + size; v[4].pos.y = cy + size; v[4].u = 1.0f; v[4].v = 1.0f;
                v[5].pos.x = cx - size; v[5].pos.y = cy + size; v[5].u = 0.0f; v[5].v = 1.0f;
                DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);

                SetUsePixelShader(-1);

                int tw = GetDrawStringWidthToHandle(bOp.symbol.c_str(), 1, m_fontNumber);
                double rotCX = (double)tw / 2.0; double rotCY = 24.0;
                DrawRotaStringToHandle((int)cx + 2, (int)cy + 2, 1.0, 1.0, rotCX, rotCY, (double)bOp.angle, GetColor(10, 15, 30), m_fontNumber, 0, FALSE, bOp.symbol.c_str());
                DrawRotaStringToHandle((int)cx, (int)cy, 1.0, 1.0, rotCX, rotCY, (double)bOp.angle, GetColor(255, 255, 255), m_fontNumber, 0, FALSE, bOp.symbol.c_str());
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        const char* titleText = "超計算マスBATTLE";
        int titleW = GetDrawStringWidthToHandle(titleText, (int)strlen(titleText), m_fontTitle);
        double t = GetNowCount() / 1000.0;
        float floatY = (float)sin(t * 2.0) * 8.0f;
        int titleX = CX - titleW / 2;
        int titleY = 180 + (int)floatY;

        SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
        for (int i = 0; i < 4; ++i) {
            int offset = i * 2;
            DrawStringToHandle(titleX - offset, titleY - offset, titleText, COL_TITLE_SUB(), m_fontTitle);
            DrawStringToHandle(titleX + offset, titleY + offset, titleText, COL_TITLE_SUB(), m_fontTitle);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        float glitchStrength = (float)sin(t * 10.0) * 3.0f;
        SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
        DrawStringToHandle(titleX + (int)glitchStrength, titleY, titleText, GetColor(255, 0, 100), m_fontTitle);
        DrawStringToHandle(titleX - (int)glitchStrength, titleY, titleText, GetColor(0, 100, 255), m_fontTitle);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawStringToHandle(titleX, titleY, titleText, COL_TITLE_MAIN(), m_fontTitle);

        float scanPos = (float)fmod(t * 1.5, 2.0) - 1.0f;
        int shineX = titleX + (int)(scanPos * titleW * 1.5f);
        SetDrawArea(titleX, titleY, titleX + titleW, titleY + 110);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 180);
        for (int i = 0; i < 20; ++i) {
            DrawLine(shineX + i, titleY, shineX + i - 30, titleY + 100, GetColor(255, 255, 255));
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        SetDrawArea(0, 0, sw, sh);

        DrawLine(CX - 500, 310, CX + 500, 310, COL_TITLE_SUB(), 5);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
        DrawLine(CX - 500, 310, CX + 500, 310, COL_TITLE_MAIN(), 2);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        int blinkAlpha = (int)(BLINK_BASE_ALPHA + BLINK_AMP_ALPHA * sin(t * M_PI * BLINK_SPEED));

        auto drawBracket = [&](int x, int y, int w, int h, unsigned int col) {
            int d = 15;
            DrawLine(x, y, x + d, y, col, 2);            DrawLine(x, y, x, y + d, col, 2);
            DrawLine(x + w - d, y, x + w, y, col, 2);   DrawLine(x + w, y, x + w, y + d, col, 2);
            DrawLine(x, y + h - d, x, y + h, col, 2);   DrawLine(x, y + h, x + d, y + h, col, 2);
            DrawLine(x + w - d, y + h, x + w, y + h, col, 2); DrawLine(x + w, y + h - d, x + w, y + h, col, 2);
            };

        bool isCustomState = (m_titleState == TitleState::BATTLE_SETUP && (m_setupStep == BattleSetup::Step::CUSTOM_P1_START || m_setupStep == BattleSetup::Step::CUSTOM_P2_START));
        int menuCX = isCustomState ? CX + MENU_CUSTOM_OFFSET_X : CX;
        int menuStartY = CY + MENU_CENTER_OFFSET_Y;

        auto drawMenuList = [&](int cursor, const char* menuTitle, const std::vector<std::string>& items) {
            if (menuTitle && strlen(menuTitle) > 0) {
                int mtW = GetDrawStringWidthToHandle(menuTitle, (int)strlen(menuTitle), m_fontMenu);
                DrawStringToHandle(menuCX - mtW / 2, menuStartY, menuTitle, COL_GRID(), m_fontMenu);
            }

            for (size_t i = 0; i < items.size(); ++i) {
                int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                unsigned int baseCol = (cursor == i) ? COL_TEXT_ON() : COL_TEXT_OFF();
                int tw = GetDrawStringWidthToHandle(items[i].c_str(), (int)items[i].length(), m_fontMenu);

                if (cursor == i) {
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha / 3);
                    DrawBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y, menuCX + MENU_BOX_HALF_W, iy + MENU_BOX_H - MENU_BOX_OFFSET_Y, baseCol, TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    drawBracket(menuCX - MENU_BOX_HALF_W, iy - 15, MENU_BOX_HALF_W * 2, 70, baseCol);
                    DrawStringToHandle(menuCX - 380, iy, " >>", baseCol, m_fontMenu);
                    DrawStringToHandle(menuCX + 335, iy, "<< ", baseCol, m_fontMenu);
                }
                DrawStringToHandle(menuCX - tw / 2, iy, items[i].c_str(), baseCol, m_fontMenu);
            }
            };

        switch (m_titleState) {

        case TitleState::PRESS_START: {
            const char* pushText = "スペースかクリックでスタート";
            int ptW = GetDrawStringWidthToHandle(pushText, (int)strlen(pushText), m_fontMenu);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha);
            DrawStringToHandle(CX - ptW / 2, CY + 100, pushText, COL_GRID(), m_fontMenu);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            break;
        }

        case TitleState::MAIN_MENU:
            drawMenuList(m_mainMenuCursor, "", { "オフラインバトル", "通信対戦", "チュートリアル", "オプション", "ゲーム終了" });
            break;

        case TitleState::NETWORK_SETUP:
            switch (m_netStep) {
            case NetSetupStep::CLIENT_WAIT_SETUP:
            {
                if (view.isNetworkHost) {
                    const char* msg = "2P(クライアント)の配置設定を待機中...";
                    int msgW = GetDrawStringWidthToHandle(msg, (int)strlen(msg), m_fontMenu);
                    DrawStringToHandle(CX - msgW / 2, CY, msg, COL_TEXT_SUB(), m_fontMenu);
                }
                else {
                    const char* msg = "ホストがゲームルールを設定中です...";
                    int msgW = GetDrawStringWidthToHandle(msg, (int)strlen(msg), m_fontMenu);
                    DrawStringToHandle(CX - msgW / 2, CY - 250, msg, COL_TEXT_SUB(), m_fontMenu);
                    m_miniGame.Draw();
                }
                break;
            }
            case NetSetupStep::SELECT_ROLE:
                drawMenuList(m_netRoleCursor, "【 通信対戦 】", { "部屋を作る (ホスト)", "部屋を探す (クライアント)" });
                break;
            case NetSetupStep::HOST_WAITING:
            {
                const char* msg = "対戦相手を待っています...";
                int msgW = GetDrawStringWidthToHandle(msg, (int)strlen(msg), m_fontMenu);
                DrawStringToHandle(CX - msgW / 2, CY, msg, COL_TEXT_SUB(), m_fontMenu);

                const char* subMsg = "(同じLAN内のPCから検索可能)";
                int subW = GetDrawStringWidthToHandle(subMsg, (int)strlen(subMsg), m_fontSmall);
                DrawStringToHandle(CX - subW / 2, CY + 60, subMsg, COL_TEXT_SUB(), m_fontSmall);
                break;
            }
            case NetSetupStep::CLIENT_SEARCHING:
            {
                if (view.hostEntries.empty()) {
                    const char* msg = "部屋を探しています...";
                    int msgW = GetDrawStringWidthToHandle(msg, (int)strlen(msg), m_fontMenu);
                    DrawStringToHandle(CX - msgW / 2, CY, msg, COL_TEXT_SUB(), m_fontMenu);
                }
                else {
                    drawMenuList(m_hostListCursor, "【 見つかった部屋 】", view.hostEntries);
                }
                break;
            }
            }
            break;

        case TitleState::OPTION_MENU:
            drawMenuList(-1, "【 オプション 】", { "※ここに音量設定などを追加", "（現在は準備中です）" });
            break;

        case TitleState::EXIT_CONFIRM:
            drawMenuList(m_exitCursor, "【 ゲームを終了しますか？ 】", { "いいえ (戻る)", "はい (終了)" });
            break;

        case TitleState::BATTLE_SETUP:
            switch (m_setupStep) {
            case BattleSetup::Step::SELECT_PLAYERS:
                drawMenuList(m_playerCursor, "【 バトル方式 】", { "シングルバトル", "オフラインバトル" });
                break;
            case BattleSetup::Step::SELECT_MODE: {
                drawMenuList(m_modeCursor, "【 プレイモード 】", { "ノーマルバトル", "カウントバトル", "ラウンドバトル" });
                const char* modeDesc = nullptr;
                if (m_modeCursor == 0) modeDesc = "相手のバッテリーを削り切れ！演算子バトルの真骨頂！上級者向け！";
                else if (m_modeCursor == 1) modeDesc = "目標値へピタリと合わせろ！わかりやすくておすすめ！！";
                else modeDesc = "ラウンドごとに数字と演算子を選び、TARGET到達を狙う新ルール！";
                int descW = GetDrawStringWidthToHandle(modeDesc, (int)strlen(modeDesc), m_fontSmall);
                int descX = menuCX - descW / 2;
                int descY = menuStartY + MENU_ITEM_BASE_Y + 3 * MENU_ITEM_STEP_Y + 20;
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
                DrawBox(descX - 20, descY - 10, descX + descW + 20, descY + 40, COL_BG(), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                const unsigned int modeCol = (m_modeCursor == 0) ? COL_P1() : (m_modeCursor == 1 ? COL_P2() : GetColor(120, 220, 255));
                DrawBox(descX - 20, descY - 10, descX + descW + 20, descY + 40, modeCol, FALSE);
                DrawStringToHandle(descX, descY, modeDesc, COL_TEXT_ON(), m_fontSmall);
                break;
            }
            case BattleSetup::Step::SELECT_CLASSIC_STOCKS:
                drawMenuList(m_stocksCursor, "【 残機設定 】", { "残機: 1", "残機: 3", "残機: 5" });
                break;
            case BattleSetup::Step::SELECT_SCORE:
                drawMenuList(m_scoreCursor, "【 目標スコア設定 】", { "目標スコア: 53", "目標スコア: 103", "目標スコア: 223" });
                break;
            case BattleSetup::Step::SELECT_P1_TYPE:
                drawMenuList(m_players[0].typeCursor, "【 1P 操作設定 】", { "プレイヤー", "NPC" });
                break;
            case BattleSetup::Step::SELECT_P2_TYPE:
                drawMenuList(m_players[1].typeCursor, "【 2P 操作設定 】", { "プレイヤー", "NPC" });
                break;
            case BattleSetup::Step::SELECT_STAGE: {
                drawMenuList(m_stageCursor, "【 ステージ選択 】", { "バランステージ", "マイナステージ", "カオステージ" });
                const char* recommendText = nullptr;
                unsigned int recommendCol = COL_TEXT_OFF();
                if (m_modeCursor == 0) {
                    if (m_stageCursor == 1) { recommendText = "おすすめ！"; recommendCol = GetColor(255, 215, 0); }
                    else { recommendText = "(マイナステージがおすすめ)"; recommendCol = GetColor(150, 150, 180); }
                }
                else { recommendText = "すべておすすめ！"; recommendCol = GetColor(100, 255, 150); }

                int recX = menuCX + 420;
                int recY = menuStartY + 50;
                if (m_modeCursor == 0 && m_stageCursor == 1) recY = menuStartY + MENU_ITEM_BASE_Y + m_stageCursor * MENU_ITEM_STEP_Y + 15;
                DrawStringToHandle(recX, recY, recommendText, recommendCol, m_fontSmall);
                break;
            }
            case BattleSetup::Step::CUSTOM_P1_START:
            case BattleSetup::Step::CUSTOM_P2_START: {
                bool is1P = (m_setupStep == BattleSetup::Step::CUSTOM_P1_START);
                int pIdx = is1P ? 0 : 1;
                auto& p = m_players[pIdx];

                std::string menuTitle = is1P ? "【 1P 初期設定 】" : "【 2P 初期設定 】";
                int mtW = GetDrawStringWidthToHandle(menuTitle.c_str(), (int)menuTitle.length(), m_fontMenu);
                DrawStringToHandle(menuCX - mtW / 2, menuStartY, menuTitle.c_str(), is1P ? COL_P1() : COL_P2(), m_fontMenu);

                const bool isRoundMode = view.setup->IsRoundBattle();
                if (isRoundMode) {
                    const int iy = menuStartY + MENU_ITEM_BASE_Y;
                    const unsigned int color = COL_TEXT_ON();
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha / 3);
                    DrawBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y, menuCX + MENU_BOX_HALF_W, iy + MENU_BOX_H - MENU_BOX_OFFSET_Y, color, TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    drawBracket(menuCX - MENU_BOX_HALF_W, iy - 15, MENU_BOX_HALF_W * 2, 70, color);
                    const char* decideText = is1P ? "初期位置を決定 (NEXT)" : "初期位置を決定 (START)";
                    const int tw = GetDrawStringWidthToHandle(decideText, (int)strlen(decideText), m_fontMenu);
                    DrawStringToHandle(menuCX - tw / 2, iy, decideText, color, m_fontMenu);
                }
                else {
                    // 旧2モードでは従来どおり、タイトルで初期パワーも設定する。
                    const char* items[2] = { "初期パワー", is1P ? "設定完了 (NEXT)" : "バトル開始 (START)" };
                    int vals[1] = { p.startNum };

                    for (int i = 0; i < 2; ++i) {
                        int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                        unsigned int color = (p.customCursor == i) ? COL_TEXT_ON() : COL_TEXT_OFF();

                        if (p.customCursor == i) {
                            SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha / 3);
                            DrawBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y, menuCX + MENU_BOX_HALF_W, iy + MENU_BOX_H - MENU_BOX_OFFSET_Y, color, TRUE);
                            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                            drawBracket(menuCX - MENU_BOX_HALF_W, iy - 15, MENU_BOX_HALF_W * 2, 70, color);
                        }

                        if (i == 0) {
                            char nameBuf[128]; sprintf_s(nameBuf, "%-20s", items[i]);
                            DrawStringToHandle(menuCX - 320, iy, nameBuf, color, m_fontMenu);
                            int valX = menuCX + CUSTOM_BTN_BASE_X;
                            bool hoverL = HoverBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE);
                            unsigned int colL = hoverL ? COL_WHITE() : color;
                            DrawBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, valX - 10, iy + 45, colL, FALSE);
                            DrawStringToHandle(valX - 45, iy, "<", colL, m_fontMenu);
                            DrawFormatStringToHandle(valX + 15, iy - 4, color, m_fontNumber, "%d", vals[i]);
                            bool hoverR = HoverBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE);
                            unsigned int colR = hoverR ? COL_WHITE() : color;
                            DrawBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, valX + 110, iy + 45, colR, FALSE);
                            DrawStringToHandle(valX + 75, iy, ">", colR, m_fontMenu);
                        }
                        else {
                            const char* decideText = items[i];
                            int tw = GetDrawStringWidthToHandle(decideText, (int)strlen(decideText), m_fontMenu);
                            DrawStringToHandle(menuCX - tw / 2, iy, decideText, color, m_fontMenu);
                            if (p.customCursor == i) {
                                DrawStringToHandle(menuCX - tw / 2 - 60, iy, " >>", color, m_fontMenu);
                                DrawStringToHandle(menuCX + tw / 2 + 20, iy, "<< ", color, m_fontMenu);
                            }
                        }
                    }
                }

                // ★ ミニマップ描画 ＆ ホバー強調
                int mapBaseX = CX + MAP_BASE_OFFSET_X;
                int mapBaseY = menuStartY + MAP_BASE_OFFSET_Y;
                DrawStringToHandle(mapBaseX + 60, mapBaseY - 50, "【 マップをクリックして配置 】", COL_TEXT_ON(), m_fontSmall);
                DrawBox(mapBaseX - 5, mapBaseY - 5, mapBaseX + MAP_CELL_SIZE * GRID_SIZE + 5, mapBaseY + MAP_CELL_SIZE * GRID_SIZE + 5, COL_GRID(), FALSE);
                DrawStringToHandle(mapBaseX - 40, mapBaseY - 40, "Y", COL_GRID(), m_fontSmall);
                DrawStringToHandle(mapBaseX + MAP_CELL_SIZE * GRID_SIZE + 15, mapBaseY + MAP_CELL_SIZE * GRID_SIZE - 10, "X", COL_GRID(), m_fontSmall);

                for (int y = 0; y < GRID_SIZE; ++y) {
                    for (int x = 0; x < GRID_SIZE; ++x) {
                        int drawX = mapBaseX + x * MAP_CELL_SIZE, drawY = mapBaseY + y * MAP_CELL_SIZE;
                        int uiX = x + 1, uiY = GRID_SIZE - y;

                        DrawBox(drawX, drawY, drawX + MAP_CELL_SIZE, drawY + MAP_CELL_SIZE, COL_BG(), TRUE);
                        DrawBox(drawX, drawY, drawX + MAP_CELL_SIZE, drawY + MAP_CELL_SIZE, COL_TITLE_SUB(), FALSE);

                        if (HoverBox(drawX, drawY, MAP_CELL_SIZE, MAP_CELL_SIZE)) {
                            SetDrawBlendMode(DX_BLENDMODE_ADD, 100);
                            DrawBox(drawX, drawY, drawX + MAP_CELL_SIZE, drawY + MAP_CELL_SIZE, COL_WHITE(), TRUE);
                            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                        }

                        // 1P, 2Pの配置位置描画
                        for (int i = 0; i < 2; ++i) {

                            // ===============================================
                            // ★ここを追加：通信・オフライン問わず、設定中は「相手の駒」を隠す！
                            // ===============================================
                            bool isOnline = view.networkConnected;
                            if (isOnline) {
                                if (is1P && i == 1) continue;  // ホスト(1P)は2Pを見れない
                                if (!is1P && i == 0) continue; // クライアント(2P)は1Pを見れない
                            }
                            else {
                                if (is1P && i == 1) continue;  // オフライン時も1P設定中は2Pを隠す
                            }
                            // ===============================================

                            // 自分の駒だけは通常通り描画する
                            if (uiX == m_players[i].startX && uiY == m_players[i].startY) {
                                int alpha = ((i == 0 && is1P) || (i == 1 && !is1P)) ? blinkAlpha : 150;
                                SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                                DrawBox(drawX + 2, drawY + 2, drawX + MAP_CELL_SIZE - 2, drawY + MAP_CELL_SIZE - 2, i == 0 ? COL_P1() : COL_P2(), TRUE);
                                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                                if (isRoundMode) {
                                    DrawStringToHandle(drawX + 8, drawY + 10, i == 0 ? "P1" : "P2", COL_BLACK(), m_fontSmall);
                                }
                                else {
                                    DrawFormatStringToHandle(drawX + 16, drawY + 10, COL_BLACK(), m_fontSmall, "%d", m_players[i].startNum);
                                }
                            }
                        }
                    }
                }
                break;
            }
            }
            break;
        }

        if (m_titleState != TitleState::PRESS_START && m_titleState != TitleState::MAIN_MENU &&
            !(m_titleState == TitleState::NETWORK_SETUP && m_netStep == NetSetupStep::CLIENT_WAIT_SETUP && !view.isNetworkHost)) {
            int backBtnX = CX - 150, backBtnY = sh - 150, backBtnW = 300, backBtnH = 60;
            bool isHover = HoverBox(backBtnX, backBtnY, backBtnW, backBtnH);
            unsigned int btnCol = isHover ? COL_TEXT_ON() : COL_TEXT_OFF();
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, isHover ? 200 : 120);
            DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, COL_BG(), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, btnCol, FALSE);
            const char* backStr = "戻る (B)";
            int bw = GetDrawStringWidthToHandle(backStr, (int)strlen(backStr), m_fontMenu);
            DrawStringToHandle(backBtnX + (backBtnW - bw) / 2, backBtnY + 10, backStr, btnCol, m_fontMenu);
        }

        DrawBox(0, sh - 70, sw, sh - 15, GetColor(10, 20, 40), TRUE);
        std::string guideText = "";
        if (m_titleState == TitleState::BATTLE_SETUP && (m_setupStep == BattleSetup::Step::CUSTOM_P1_START || m_setupStep == BattleSetup::Step::CUSTOM_P2_START)) {
            if (view.setup->IsRoundBattle()) guideText = "MAP CLICK: 初期位置変更   |   [SPACE]/CLICK: 決定";
            else guideText = "[↑][↓]/CLICK: 項目選択   |   [<][>]/CLICK: 数値変更   |   MAP CLICK: 初期位置変更";
        }
        else if (m_titleState != TitleState::PRESS_START) {
            guideText = "[↑][↓]/CLICK: 項目選択   |   [SPACE]: 決定";
        }

        if (!guideText.empty()) {
            int gw = GetDrawStringWidthToHandle(guideText.c_str(), (int)guideText.length(), m_fontSmall);
            int guideX = isCustomState ? (CX - gw / 2 - 100) : (CX - gw / 2);
            DrawStringToHandle(guideX, sh - 55, guideText.c_str(), COL_TEXT_OFF(), m_fontSmall);
        }

        const char* fullscreenGuide = "[F11] 全画面表示 / ウィンドウ切替";
        int fsGuideW = GetDrawStringWidthToHandle(fullscreenGuide, (int)strlen(fullscreenGuide), m_fontSmall);
        DrawStringToHandle(sw - fsGuideW - 20, sh - 55, fullscreenGuide, GetColor(150, 150, 180), m_fontSmall);

        if (m_psImpactHandle != -1 && m_cbImpactHandle != -1) {
            SetUsePixelShader(m_psImpactHandle);
            SetDrawBlendMode(DX_BLENDMODE_ADD, 255);

            for (const auto& r : m_ripples) {
                float progress = 1.0f - (r.alpha / 220.0f);
                if (progress < 0.0f) progress = 0.0f;
                if (progress > 1.0f) progress = 1.0f;

                float cr = ((r.color >> 16) & 0xFF) / 255.0f;
                float cg = ((r.color >> 8) & 0xFF) / 255.0f;
                float cb = ((r.color) & 0xFF) / 255.0f;

                float* shaderParams = (float*)GetBufferShaderConstantBuffer(m_cbImpactHandle);
                shaderParams[0] = progress;
                shaderParams[1] = cr; shaderParams[2] = cg; shaderParams[3] = cb;
                shaderParams[4] = (float)m_impactType;
                shaderParams[5] = r.x;
                shaderParams[6] = 0.0f; shaderParams[7] = 0.0f;

                UpdateShaderConstantBuffer(m_cbImpactHandle);
                SetShaderConstantBuffer(m_cbImpactHandle, DX_SHADERTYPE_PIXEL, 0);

                float size = 300.0f;
                VERTEX2DSHADER v[6];
                for (int i = 0; i < 6; ++i) {
                    v[i].pos = VGet(0, 0, 0); v[i].rhw = 1.0f;
                    v[i].dif = GetColorU8(255, 255, 255, 255);
                    v[i].spc = GetColorU8(0, 0, 0, 0);
                }
                v[0].pos.x = r.x - size; v[0].pos.y = r.y - size; v[0].u = 0.0f; v[0].v = 0.0f;
                v[1].pos.x = r.x + size; v[1].pos.y = r.y - size; v[1].u = 1.0f; v[1].v = 0.0f;
                v[2].pos.x = r.x - size; v[2].pos.y = r.y + size; v[2].u = 0.0f; v[2].v = 1.0f;
                v[3].pos.x = r.x + size; v[3].pos.y = r.y - size; v[3].u = 1.0f; v[3].v = 0.0f;
                v[4].pos.x = r.x + size; v[4].pos.y = r.y + size; v[4].u = 1.0f; v[4].v = 1.0f;
                v[5].pos.x = r.x - size; v[5].pos.y = r.y + size; v[5].u = 0.0f; v[5].v = 1.0f;

                DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
            }
            SetUsePixelShader(-1);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    void TitleUI::Release() {
        if (m_fontTitle != -1) { DeleteFontToHandle(m_fontTitle); m_fontTitle = -1; }
        if (m_fontMenu != -1) { DeleteFontToHandle(m_fontMenu); m_fontMenu = -1; }
        if (m_fontSmall != -1) { DeleteFontToHandle(m_fontSmall); m_fontSmall = -1; }
        if (m_fontNumber != -1) { DeleteFontToHandle(m_fontNumber); m_fontNumber = -1; }

        if (m_psHandle != -1) { DeleteShader(m_psHandle); m_psHandle = -1; }
        if (m_cbHandle != -1) { DeleteShaderConstantBuffer(m_cbHandle); m_cbHandle = -1; }
        if (m_psCrystalHandle != -1) { DeleteShader(m_psCrystalHandle); m_psCrystalHandle = -1; }
        if (m_cbCrystalHandle != -1) { DeleteShaderConstantBuffer(m_cbCrystalHandle); m_cbCrystalHandle = -1; }
        if (m_psImpactHandle != -1) { DeleteShader(m_psImpactHandle); m_psImpactHandle = -1; }
        if (m_cbImpactHandle != -1) { DeleteShaderConstantBuffer(m_cbImpactHandle); m_cbImpactHandle = -1; }
    }

} // namespace App
