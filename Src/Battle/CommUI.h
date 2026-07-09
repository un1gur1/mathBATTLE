#pragma once
#include "../Manager/NetworkManager.h"
#include <string>
#include <vector>

namespace App {

    struct CommLog {
        bool isText;
        bool isMine;            // 自分か相手か（色とスライド方向の判定用）
        std::string message;
        int pointCount;
        int pointsX[500];
        int pointsY[500];
        int displayTimer;       // 寿命タイマー

        // アニメーション用変数
        float currentX;
        float targetX;
        float currentY;
        float targetY;
    };

    class CommUI {
    public:
        CommUI();
        ~CommUI();

        void Init();
        void Update();
        void Draw() const;

        bool IsTyping() const { return m_isTyping; }

    private:
        std::vector<CommLog> m_logs;

        bool m_isTyping;
        char m_inputBuf[256];
        int  m_inputStringEnd;

        bool m_isDrawing;
        StampPacket m_currentDrawing;

        // 内部フォントハンドル
        int m_fontChat;

        void UpdateIncomingPackets();
        void UpdateOutgoingInputs();

        // サイバー風の枠を描画するヘルパー
        void DrawCyberBracket(int x, int y, int w, int h, unsigned int col) const;
    };

} // namespace App