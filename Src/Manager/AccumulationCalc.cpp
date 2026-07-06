#define NOMINMAX
#include "AccumulationCalc.h"
#include <cmath>
#include <string>
#include <algorithm> 

AccumulationCalc::AccumulationCalc() {
    int sw, sh;
    GetDrawScreenSize(&sw, &sh);
    m_centerX = sw / 2;
    m_centerY = sh / 2 + 60;
    m_prevMouse = 0;
    m_clearCount = 0;

    SRand(GetNowCount());

    InitFonts();
    InitButtons();
    NextStage(false);
}

AccumulationCalc::~AccumulationCalc() {
    DeleteFontToHandle(m_fontBtn);
    DeleteFontToHandle(m_fontDisp);
    DeleteFontToHandle(m_fontTotal);
}

void AccumulationCalc::InitFonts() {
    m_fontBtn = CreateFontToHandle("BIZ UDゴシック", 50, 2, DX_FONTTYPE_ANTIALIASING);
    m_fontDisp = CreateFontToHandle("BIZ UDゴシック", 64, 3, DX_FONTTYPE_ANTIALIASING);
    m_fontTotal = CreateFontToHandle("BIZ UDゴシック", 38, 2, DX_FONTTYPE_ANTIALIASING);
}

void AccumulationCalc::InitButtons() {
    int panelW = 680;
    int panelH = 540;
    int pX = m_centerX - panelW / 2;
    int pY = m_centerY - panelH / 2;

    int startX = pX + 50;
    int startY = pY + 230; // ボタン位置を少し下げてディスプレイを広げました
    int stepX = 120;
    int stepY = 100;
    int btnW = 100;
    int btnH = 80;

    struct BtnData { int r, c; const char* lbl; int t, v; };
    BtnData data[15] = {
        {0,0,"7",0,7}, {0,1,"8",0,8}, {0,2,"9",0,9}, {0,3,"+",1,'+'}, {0,4,"-",1,'-'},
        {1,0,"4",0,4}, {1,1,"5",0,5}, {1,2,"6",0,6}, {1,3,"*",1,'*'}, {1,4,"/",1,'/'},
        {2,0,"1",0,1}, {2,1,"2",0,2}, {2,2,"3",0,3}, {2,3,"C",3,0},   {2,4,"=",2,0}
    };

    for (int i = 0; i < 15; ++i) {
        m_buttons[i].x = startX + data[i].c * stepX;
        m_buttons[i].y = startY + data[i].r * stepY;
        m_buttons[i].w = btnW;
        m_buttons[i].h = btnH;
        strcpy_s(m_buttons[i].label, data[i].lbl);
        m_buttons[i].type = data[i].t;
        m_buttons[i].val = data[i].v;
    }
}

void AccumulationCalc::NextStage(bool isClear) {
    if (isClear) m_clearCount++;
    else m_clearCount = 0;

    Reset();

    int primeTargets[] = { 43, 53, 71, 89, 103, 137, 149, 199, 223, 277, 311, 353, 401, 449, 499 };
    int minIdx = std::min(m_clearCount, 5);
    int maxIdx = std::min(minIdx + 5, 14);

    int idx = minIdx + GetRand(maxIdx - minIdx);
    m_targetScore = primeTargets[idx];
}

void AccumulationCalc::Reset() {
    m_runningTotal = 0;
    m_num1 = 0;
    m_num2 = 0;
    m_op = '\0';
    m_inputPhase = 0;
    m_isCleared = false;
}

void AccumulationCalc::PushNumber(int n) {
    if (n <= 0 || n > 9) return;
    if (m_inputPhase == 0) m_num1 = n;
    else m_num2 = n;
}

void AccumulationCalc::PushOperator(char op) {
    if (m_num1 != 0) {
        m_op = op;
        m_inputPhase = 1;
    }
}

void AccumulationCalc::PushEqual() {
    if (m_inputPhase == 1 && m_num2 != 0) {
        int res = 0;
        switch (m_op) {
        case '+': res = m_num1 + m_num2; break;
        case '-': res = m_num1 - m_num2; break;
        case '*': res = m_num1 * m_num2; break;
        case '/': if (m_num2 != 0) res = m_num1 / m_num2; break; // 0除算防止
        }
        m_runningTotal += res; // 結果をTOTALに合算！(マイナスなら引き算になる)

        // 計算が終わったら数式をリセット
        m_num1 = 0;
        m_num2 = 0;
        m_op = '\0';
        m_inputPhase = 0;

        if (m_runningTotal == m_targetScore) {
            m_isCleared = true;
        }
    }
}

void AccumulationCalc::Update() {
    int mouse = GetMouseInput();
    bool isClick = (mouse & MOUSE_INPUT_LEFT) && !(m_prevMouse & MOUSE_INPUT_LEFT);
    m_prevMouse = mouse;

    if (isClick) {
        if (m_isCleared) {
            NextStage(true);
            return;
        }

        int mx, my;
        GetMousePoint(&mx, &my);

        for (int i = 0; i < 15; ++i) {
            if (mx >= m_buttons[i].x && mx <= m_buttons[i].x + m_buttons[i].w &&
                my >= m_buttons[i].y && my <= m_buttons[i].y + m_buttons[i].h) {

                switch (m_buttons[i].type) {
                case 0: PushNumber(m_buttons[i].val); break;
                case 1: PushOperator((char)m_buttons[i].val); break;
                case 2: PushEqual(); break;
                case 3: NextStage(false); break; // [C]でギブアップ＆リセット
                }
                break;
            }
        }
    }
}

void AccumulationCalc::Draw() {
    int colBg = GetColor(10, 12, 18);
    int colPanel = GetColor(18, 20, 28);
    int colCyber = GetColor(0, 150, 255);
    int colAccent = GetColor(255, 140, 0);
    int colText = GetColor(255, 255, 255);   // ★修正：数式をハッキリ見せるための純白
    int colDim = GetColor(120, 140, 170);
    int colSafe = GetColor(100, 255, 150);
    int colDanger = GetColor(255, 100, 100);

    int panelW = 680;
    int panelH = 540;
    int pX = m_centerX - panelW / 2;
    int pY = m_centerY - panelH / 2;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 245);
    DrawRoundRect(pX, pY, pX + panelW, pY + panelH, 20, 20, colBg, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawRoundRect(pX, pY, pX + panelW, pY + panelH, 20, 20, colCyber, FALSE);

    // ★修正：ディスプレイ領域の縦幅を少し広げて、3段構成を見やすくしました
    DrawRoundRect(pX + 25, pY + 25, pX + panelW - 25, pY + 215, 15, 15, colPanel, TRUE);
    DrawRoundRect(pX + 25, pY + 25, pX + panelW - 25, pY + 215, 15, 15, GetColor(30, 50, 80), FALSE);

    // ----------------------------------------------------
    // 上段：TARGET と SCORE
    // ----------------------------------------------------
    DrawFormatStringToHandle(pX + 40, pY + 35, colCyber, m_fontTotal, "TARGET: %d", m_targetScore);
    DrawFormatStringToHandle(pX + panelW - 210, pY + 35, colAccent, m_fontTotal, "SCORE: %d", m_clearCount);

    if (m_isCleared) {
        DrawFormatStringToHandle(pX + 220, pY + 90, colSafe, m_fontDisp, "CLEAR!!");

        int blinkAlpha = (int)(150 + 100 * std::sin(GetNowCount() / 150.0));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha);
        DrawStringToHandle(pX + 170, pY + 160, ">> Click to Next <<", colCyber, m_fontTotal);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    else {
        // ----------------------------------------------------
        // 中段：数式 と 計算結果のプレビュー
        // ----------------------------------------------------
        int midY = pY + 85;
        std::string eqStr = ">> ";

        if (m_num1 == 0) eqStr += "?";
        else eqStr += std::to_string(m_num1);

        if (m_op != '\0') {
            eqStr += " ";
            eqStr += m_op;
            eqStr += " ";

            if (m_num2 == 0) eqStr += "?";
            else eqStr += std::to_string(m_num2);

            // num2まで入力されたら、その横に「= 結果」をリアルタイム表示！
            if (m_num2 != 0) {
                int tempRes = 0;
                switch (m_op) {
                case '+': tempRes = m_num1 + m_num2; break;
                case '-': tempRes = m_num1 - m_num2; break;
                case '*': tempRes = m_num1 * m_num2; break;
                case '/': tempRes = (m_num2 != 0) ? m_num1 / m_num2 : 0; break;
                }
                eqStr += " = ";
                eqStr += std::to_string(tempRes);
            }
        }

        // ★修正：薄色ではなく、ハッキリとした色（colText）で大きく描画
        DrawStringToHandle(pX + 40, midY, eqStr.c_str(), colText, m_fontDisp);

        // ----------------------------------------------------
        // 下段：TOTAL と 差分
        // ----------------------------------------------------
        int btmY = pY + 160;
        int diff = m_targetScore - m_runningTotal;
        unsigned int diffCol = colDim;

        if (diff < 0) {
            diffCol = colDanger;
        }
        else if (diff <= 10) {
            diffCol = colAccent;
        }

        // TOTALを左側に描画
        DrawFormatStringToHandle(pX + 45, btmY, colText, m_fontTotal, "TOTAL: %d", m_runningTotal);

        // 差分を右側に描画
        if (diff > 0) {
            DrawFormatStringToHandle(pX + 350, btmY, diffCol, m_fontTotal, "(あと: %d)", diff);
        }
        else if (diff < 0) {
            DrawFormatStringToHandle(pX + 350, btmY, diffCol, m_fontTotal, "(ｵｰﾊﾞｰ: %d)", -diff);
        }
        else {
            DrawFormatStringToHandle(pX + 350, btmY, colSafe, m_fontTotal, "(JUST !!)");
        }
    }

    // ----------------------------------------------------
    // ボタンの描画
    // ----------------------------------------------------
    int mx, my;
    GetMousePoint(&mx, &my);

    for (int i = 0; i < 15; ++i) {
        int bx = m_buttons[i].x;
        int by = m_buttons[i].y;
        int bw = m_buttons[i].w;
        int bh = m_buttons[i].h;

        bool isHover = !m_isCleared && (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh);
        unsigned int drawCol = m_isCleared ? colDim : colCyber;

        int textW = GetDrawStringWidthToHandle(m_buttons[i].label, (int)strlen(m_buttons[i].label), m_fontBtn);
        int textX = bx + (bw - textW) / 2;
        int textY = by + (bh - 50) / 2;

        if (isHover) {
            DrawRoundRect(bx, by, bx + bw, by + bh, 15, 15, drawCol, TRUE);
            DrawStringToHandle(textX, textY, m_buttons[i].label, colBg, m_fontBtn);
        }
        else {
            DrawRoundRect(bx, by, bx + bw, by + bh, 15, 15, drawCol, FALSE);
            DrawStringToHandle(textX, textY, m_buttons[i].label, m_isCleared ? colDim : colText, m_fontBtn);
        }
    }
}