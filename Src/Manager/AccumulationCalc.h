#pragma once
#include "DxLib.h"

class AccumulationCalc {
private:
    int m_num1;
    int m_num2;
    char m_op;
    int m_inputPhase;
    int m_latestResult;
    int m_runningTotal;
    bool m_hasResult;

    int m_centerX;
    int m_centerY;
    int m_prevMouse;

    // ★追加：綺麗なアンチエイリアスフォント用のハンドル
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
    AccumulationCalc(); // 引数なしで自動的に画面中央を取得するように変更
    ~AccumulationCalc(); // ★追加：使い終わったらフォントを削除する
    void Clear();
    void PushNumber(int n);
    void PushOperator(char op);
    void PushEqual();
    void Update();
    void Draw();
};