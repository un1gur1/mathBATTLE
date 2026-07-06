#pragma once
#include "DxLib.h"

class AccumulationCalc {
private:
    int m_num1;          // 入力する1つめの数字
    int m_num2;          // 入力する2つめの数字
    char m_op;           // 演算子
    int m_inputPhase;    // 0:num1入力待ち, 1:num2入力待ち

    int m_runningTotal;  // 現在のトータルスコア
    int m_targetScore;   // 目標スコア

    bool m_isCleared;    // クリアフラグ
    int m_clearCount;    // 連勝数（スコア）

    int m_centerX;
    int m_centerY;
    int m_prevMouse;

    int m_fontBtn;
    int m_fontDisp;
    int m_fontTotal;

    struct Button {
        int x, y, w, h;
        char label[4];
        int type;
        int val;
    };
    Button m_buttons[15];

    void InitButtons();
    void InitFonts();

public:
    AccumulationCalc();
    ~AccumulationCalc();
    void NextStage(bool isClear);
    void Reset();
    void PushNumber(int n);
    void PushOperator(char op);
    void PushEqual();
    void Update();
    void Draw();
};