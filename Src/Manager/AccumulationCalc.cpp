#include "AccumulationCalc.h"

AccumulationCalc::AccumulationCalc() {
    // ★ 実際の描画画面サイズを取得して、確実にド真ん中を計算する！
    int sw, sh;
    GetDrawScreenSize(&sw, &sh);
    m_centerX = sw / 2;
    m_centerY = sh / 2;
    m_prevMouse = 0;

    InitFonts();
    InitButtons();
    Clear();
}

AccumulationCalc::~AccumulationCalc() {
    // 使い終わったらフォントメモリを解放（メモリリーク防止）
    DeleteFontToHandle(m_fontBtn);
    DeleteFontToHandle(m_fontDisp);
    DeleteFontToHandle(m_fontTotal);
}

void AccumulationCalc::InitFonts() {
    // ★ めちゃくちゃ綺麗なアンチエイリアスフォントを生成
    m_fontBtn = CreateFontToHandle("BIZ UDゴシック", 50, 2, DX_FONTTYPE_ANTIALIASING);
    m_fontDisp = CreateFontToHandle("BIZ UDゴシック", 64, 3, DX_FONTTYPE_ANTIALIASING);
    m_fontTotal = CreateFontToHandle("BIZ UDゴシック", 42, 2, DX_FONTTYPE_ANTIALIASING);
}

void AccumulationCalc::InitButtons() {
    // ★ パネルサイズを大幅アップ（幅540, 高さ720）
    int pX = m_centerX - 270;
    int pY = m_centerY - 360;

    // テンキーの配置設定 (ボタンサイズ95, 余白を含めたステップ120)
    int startX = pX + 35;
    int startY = pY + 230;
    int step = 120;
    int btnSize = 95;

    struct BtnData { int r, c; const char* lbl; int t, v; };
    BtnData data[15] = {
        {0,0,"7",0,7}, {0,1,"8",0,8}, {0,2,"9",0,9}, {0,3,"+",1,'+'},
        {1,0,"4",0,4}, {1,1,"5",0,5}, {1,2,"6",0,6}, {1,3,"-",1,'-'},
        {2,0,"1",0,1}, {2,1,"2",0,2}, {2,2,"3",0,3}, {2,3,"*",1,'*'},
        {3,0,"C",3,0}, {3,1,"=",2,0},                {3,3,"/",1,'/'}
    };

    for (int i = 0; i < 15; ++i) {
        m_buttons[i].x = startX + data[i].c * step;
        m_buttons[i].y = startY + data[i].r * step;
        m_buttons[i].w = (data[i].t == 2) ? (step * 2 - (step - btnSize)) : btnSize; // [=]ボタンは横長
        m_buttons[i].h = btnSize;
        strcpy_s(m_buttons[i].label, data[i].lbl);
        m_buttons[i].type = data[i].t;
        m_buttons[i].val = data[i].v;
    }
}

void AccumulationCalc::Clear() {
    m_num1 = 0; m_num2 = 0;
    m_op = ' ';
    m_inputPhase = 0;
    m_latestResult = 0;
    m_runningTotal = 0;
    m_hasResult = false;
}

void AccumulationCalc::PushNumber(int n) {
    if (n <= 0 || n > 9) return;
    if (m_inputPhase == 0) m_num1 = m_num1 * 10 + n;
    else m_num2 = m_num2 * 10 + n;
}

void AccumulationCalc::PushOperator(char op) {
    if (m_inputPhase == 0) {
        m_op = op;
        m_inputPhase = 1;
    }
}

void AccumulationCalc::PushEqual() {
    if (m_inputPhase == 1) {
        int res = 0;
        switch (m_op) {
        case '+': res = m_num1 + m_num2; break;
        case '-': res = m_num1 - m_num2; break;
        case '*': res = m_num1 * m_num2; break;
        case '/': if (m_num2 != 0) res = m_num1 / m_num2; break;
        }
        m_latestResult = res;
        m_runningTotal += res;
        m_hasResult = true;

        m_num1 = 0; m_num2 = 0;
        m_op = ' '; m_inputPhase = 0;
    }
}

void AccumulationCalc::Update() {
    int mouse = GetMouseInput();
    bool isClick = (mouse & MOUSE_INPUT_LEFT) && !(m_prevMouse & MOUSE_INPUT_LEFT);
    m_prevMouse = mouse;

    if (isClick) {
        int mx, my;
        GetMousePoint(&mx, &my);

        for (int i = 0; i < 15; ++i) {
            if (mx >= m_buttons[i].x && mx <= m_buttons[i].x + m_buttons[i].w &&
                my >= m_buttons[i].y && my <= m_buttons[i].y + m_buttons[i].h) {

                switch (m_buttons[i].type) {
                case 0: PushNumber(m_buttons[i].val); break;
                case 1: PushOperator((char)m_buttons[i].val); break;
                case 2: PushEqual(); break;
                case 3: Clear(); break;
                }
                break;
            }
        }
    }
}

void AccumulationCalc::Draw() {
    int colBg = GetColor(20, 22, 28);
    int colPanel = GetColor(30, 32, 40);
    int colYamabuki = GetColor(255, 177, 27);
    int colText = GetColor(245, 245, 250);
    int colDim = GetColor(150, 150, 160);

    int panelW = 540;
    int panelH = 720;
    int pX = m_centerX - panelW / 2;
    int pY = m_centerY - panelH / 2;

    // 1. 全体の背景（少しだけ透かすとサイバー感が出ます）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 245);
    DrawRoundRect(pX, pY, pX + panelW, pY + panelH, 25, 25, colBg, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawRoundRect(pX, pY, pX + panelW, pY + panelH, 25, 25, colYamabuki, FALSE);

    // 2. ディスプレイ領域
    DrawRoundRect(pX + 30, pY + 30, pX + panelW - 30, pY + 190, 15, 15, colPanel, TRUE);

    // 入力中の数式をアンチエイリアスフォントで描画
    if (m_inputPhase == 0) {
        if (m_num1 > 0) DrawFormatStringToHandle(pX + 50, pY + 50, colText, m_fontDisp, "%d", m_num1);
    }
    else {
        DrawFormatStringToHandle(pX + 50, pY + 50, colText, m_fontDisp, "%d %c %d", m_num1, m_op, m_num2 > 0 ? m_num2 : 0);
    }

    // 右下にストックされた結果を描画
    if (m_hasResult) {
        DrawFormatStringToHandle(pX + 340, pY + 40, colDim, m_fontTotal, "+) %d", m_latestResult);
        DrawFormatStringToHandle(pX + 260, pY + 110, colYamabuki, m_fontTotal, "TOTAL: %d", m_runningTotal);
    }

    // 3. ボタンの描画
    int mx, my;
    GetMousePoint(&mx, &my);

    for (int i = 0; i < 15; ++i) {
        int bx = m_buttons[i].x;
        int by = m_buttons[i].y;
        int bw = m_buttons[i].w;
        int bh = m_buttons[i].h;

        bool isHover = (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh);

        // 文字の横幅を取得して、ボタンのド真ん中に文字が来るように計算
        int textW = GetDrawStringWidthToHandle(m_buttons[i].label, (int)strlen(m_buttons[i].label), m_fontBtn);
        int textX = bx + (bw - textW) / 2;
        int textY = by + (bh - 50) / 2; // フォントサイズ50に基づくY座標補正

        if (isHover) {
            DrawRoundRect(bx, by, bx + bw, by + bh, 15, 15, colYamabuki, TRUE);
            DrawStringToHandle(textX, textY, m_buttons[i].label, colBg, m_fontBtn);
        }
        else {
            DrawRoundRect(bx, by, bx + bw, by + bh, 15, 15, colYamabuki, FALSE);
            DrawStringToHandle(textX, textY, m_buttons[i].label, colText, m_fontBtn);
        }
    }
}