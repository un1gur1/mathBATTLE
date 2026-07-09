#include "CommUI.h"
#include <DxLib.h>
#include <cstring>
#include "../Manager/ProceduralAudio.h"

namespace App {

    CommUI::CommUI() : m_isTyping(false), m_inputStringEnd(0), m_isDrawing(false), m_fontChat(-1) {
        std::memset(m_inputBuf, 0, sizeof(m_inputBuf));
    }

    CommUI::~CommUI() {
        if (m_fontChat != -1) DeleteFontToHandle(m_fontChat);
    }

    void CommUI::Init() {
        m_logs.clear();
        m_isTyping = false;
        m_isDrawing = false;
        m_inputStringEnd = 0;
        std::memset(m_inputBuf, 0, sizeof(m_inputBuf));

        // チャット用の少し小さめのフォント
        if (m_fontChat == -1) {
            m_fontChat = CreateFontToHandle("BIZ UDゴシック", 20, 2, DX_FONTTYPE_ANTIALIASING);
        }

        // ==========================================
         // ★修正：IME（予測変換）の色を完全サイバー化！
         // ==========================================

         // 1. 変換リストのドロップダウン枠全体（前回エラーになった部分の正解です！）
        SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_CONV_WIN_BACK, GetColor(10, 15, 30));
        SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_CONV_WIN_EDGE, GetColor(0, 255, 255));
        SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_CONV_WIN_STR, GetColor(255, 255, 255));
        SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_CONV_WIN_STR_EDGE, GetColor(0, 0, 0));

        // 2. 変換リストの中で「今選んでいる行」（赤色だった原因をシアン背景＋黒文字に上書き！）
        SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_CONV_WIN_SELECT_STR, GetColor(0, 0, 0));
        SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_CONV_WIN_SELECT_STR_EDGE, GetColor(0, 255, 255));
        SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_CONV_WIN_SELECT_STR_BACK, GetColor(0, 255, 255));

        //// 3. 画面に入力中の通常の文字
        //SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_STR, GetColor(255, 255, 255));
        //SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_STR_EDGE, GetColor(0, 0, 0));

        //// 4. 入力中の文字で「今まさに変換対象になっている部分」（黄色で読めなかった部分をシアン背景＋黒文字に！）
        //SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_SELECT_STR, GetColor(0, 0, 0));
        //SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_SELECT_STR_EDGE, GetColor(0, 255, 255));
        //SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_SELECT_STR_BACK, GetColor(0, 255, 255));

        // 5. カーソルと下線
        SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_CURSOR, GetColor(0, 255, 255));
        SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_LINE, GetColor(0, 255, 255));

        // 6. 「あ」や「A」などの入力モード文字
        SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_MODE_STR, GetColor(0, 255, 255));
        SetKeyInputStringColor2(DX_KEYINPSTRCOLOR_IME_MODE_STR_EDGE, GetColor(0, 0, 0));
    }

    void CommUI::Update() {
        UpdateIncomingPackets();
        UpdateOutgoingInputs();

        // ログの表示位置（Y座標）の計算用
        float currentChatY_Mine = 650.0f; // バトルログの上に配置
        float currentChatY_Opp = 650.0f;

        // アニメーションと寿命の更新
        for (auto it = m_logs.rbegin(); it != m_logs.rend(); ++it) {
            it->displayTimer--;

            // テキストチャットの場合は下から上へ押し上げる目標Y座標を設定
            if (it->isText) {
                if (it->isMine) {
                    it->targetY = currentChatY_Mine;
                    currentChatY_Mine -= 45.0f; // 1つのフキダシの高さ分ズレる
                }
                else {
                    it->targetY = currentChatY_Opp;
                    currentChatY_Opp -= 45.0f;
                }
            }

            // 残り30フレームを切ったら、画面外へスライドアウト（フェードアウト）
            if (it->displayTimer < 30) {
                it->targetX = it->isMine ? -500.0f : 2400.0f;
            }

            // イージング（滑らかな移動）
            it->currentX += (it->targetX - it->currentX) * 0.2f;
            it->currentY += (it->targetY - it->currentY) * 0.2f;
        }

        // 寿命が尽きたログの削除
        for (auto it = m_logs.begin(); it != m_logs.end(); ) {
            if (it->displayTimer <= 0) it = m_logs.erase(it);
            else ++it;
        }
    }

    void CommUI::UpdateIncomingPackets() {
        auto* net = NetworkManager::GetInstance();
        if (net->GetState() != NetworkManager::State::CONNECTED) return;

        ChatPacket recvChat;
        while (net->ReceiveChatPacket(recvChat)) {
            CommLog log;
            log.isText = true;
            log.isMine = false;
            log.message = recvChat.message;
            log.displayTimer = 420; // 7秒
            log.currentX = 1920.0f; // 画面右外から
            log.targetX = 1380.0f;  // 2P側のパネル位置へ
            log.currentY = 650.0f;
            log.targetY = 650.0f;
            m_logs.push_back(log);
            ProceduralAudio::GetInstance().PlayPowerSE(2); // 受信音
        }

        StampPacket recvStamp;
        while (net->ReceiveStampPacket(recvStamp)) {
            CommLog log;
            log.isText = false;
            log.isMine = false;
            log.pointCount = recvStamp.pointCount;
            std::memcpy(log.pointsX, recvStamp.pointsX, sizeof(int) * 500);
            std::memcpy(log.pointsY, recvStamp.pointsY, sizeof(int) * 500);
            log.displayTimer = 300;
            log.currentX = 0.0f;
            log.targetX = 0.0f;
            m_logs.push_back(log);
            ProceduralAudio::GetInstance().PlayPowerSE(5);
        }
    }

    void CommUI::UpdateOutgoingInputs() {
        auto* net = NetworkManager::GetInstance();
        bool isConnected = (net->GetState() == NetworkManager::State::CONNECTED);

        // Tキーでチャット入力開始
        if (CheckHitKey(KEY_INPUT_T) && !m_isTyping && !m_isDrawing) {
            m_isTyping = true;
            std::memset(m_inputBuf, 0, sizeof(m_inputBuf));
            m_inputStringEnd = MakeKeyInput(40, TRUE, FALSE, FALSE, FALSE);
            SetActiveKeyInput(m_inputStringEnd);
        }

        if (m_isTyping) {
            int inputState = CheckKeyInput(m_inputStringEnd);
            if (inputState == 1) { // Enterで確定
                GetKeyInputString(m_inputBuf, m_inputStringEnd);
                DeleteKeyInput(m_inputStringEnd);
                m_isTyping = false;

                if (std::strlen(m_inputBuf) > 0) {
                    if (isConnected) {
                        ChatPacket packet;
                        // ★修正：strcpy_s に変更して安全に！
                        strcpy_s(packet.message, sizeof(packet.message), m_inputBuf);
                        net->SendChatPacket(packet);
                    }
                    CommLog log;
                    log.isText = true;
                    log.isMine = true;
                    log.message = std::string(m_inputBuf);
                    log.displayTimer = 420;
                    log.currentX = -400.0f; // 画面左外から
                    log.targetX = 40.0f;    // 1P側のパネル位置へ
                    log.currentY = 650.0f;
                    log.targetY = 650.0f;
                    m_logs.push_back(log);
                }
            }
            else if (inputState == 2) { // Cancel
                DeleteKeyInput(m_inputStringEnd);
                m_isTyping = false;
            }
            return;
        }

        // 右クリックでのスタンプ描画
        int mouseInput = GetMouseInput();
        if (mouseInput & MOUSE_INPUT_RIGHT) {
            int mouseX, mouseY;
            GetMousePoint(&mouseX, &mouseY);

            // 盤面エリア（中央）でのみお絵描きできるように制限
            if (mouseX > 580 && mouseX < 1340 && mouseY > 70 && mouseY < 830) {
                if (!m_isDrawing) {
                    m_isDrawing = true;
                    m_currentDrawing.pointCount = 0;
                }
                int idx = m_currentDrawing.pointCount;
                if (idx < 500) {
                    if (idx == 0 || (m_currentDrawing.pointsX[idx - 1] != mouseX || m_currentDrawing.pointsY[idx - 1] != mouseY)) {
                        m_currentDrawing.pointsX[idx] = mouseX;
                        m_currentDrawing.pointsY[idx] = mouseY;
                        m_currentDrawing.pointCount++;
                    }
                }
            }
        }
        else {
            if (m_isDrawing) {
                if (m_currentDrawing.pointCount > 1) {
                    if (isConnected) net->SendStampPacket(m_currentDrawing);

                    CommLog log;
                    log.isText = false;
                    log.isMine = true;
                    log.pointCount = m_currentDrawing.pointCount;
                    std::memcpy(log.pointsX, m_currentDrawing.pointsX, sizeof(int) * 500);
                    std::memcpy(log.pointsY, m_currentDrawing.pointsY, sizeof(int) * 500);
                    log.displayTimer = 300;
                    log.currentX = 0.0f;
                    log.targetX = 0.0f;
                    m_logs.push_back(log);
                }
                m_isDrawing = false;
            }
        }
    }

    void CommUI::DrawCyberBracket(int x, int y, int w, int h, unsigned int col) const {
        int d = 10;
        DrawLine(x, y, x + d, y, col, 2);           DrawLine(x, y, x, y + d, col, 2);
        DrawLine(x + w - d, y, x + w, y, col, 2);   DrawLine(x + w, y, x + w, y + d, col, 2);
        DrawLine(x, y + h - d, x, y + h, col, 2);   DrawLine(x, y + h, x + d, y + h, col, 2);
        DrawLine(x + w - d, y + h, x + w, y + h, col, 2); DrawLine(x + w, y + h - d, x + w, y + h, col, 2);
    }

    void CommUI::Draw() const {
        // ==========================================
        // 1. チャットとスタンプの描画
        // ==========================================
        for (const auto& log : m_logs) {
            // フェードアウトの計算
            int alpha = (log.displayTimer > 30) ? 255 : (log.displayTimer * 8);

            if (log.isText) {
                // 1P（自分）はオレンジ、2P（相手）はブルー
                unsigned int baseCol = log.isMine ? GetColor(255, 165, 0) : GetColor(60, 150, 255);
                int x = (int)log.currentX;
                int y = (int)log.currentY;
                int w = 500;
                int h = 36;

                // ホログラム風の背景
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha * 0.8);
                DrawBox(x, y, x + w, y + h, GetColor(10, 15, 20), TRUE);
                DrawBox(x, y, x + w, y + h, baseCol, FALSE);

                // 発光するアクセントライン（自分は左、相手は右）
                SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
                if (log.isMine) DrawBox(x, y, x + 6, y + h, baseCol, TRUE);
                else DrawBox(x + w - 6, y, x + w, y + h, baseCol, TRUE);

                // テキストと「>>>」プロンプト
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                const char* prefix = log.isMine ? ">>>" : "<<<";
                DrawStringToHandle(x + 15, y + 8, prefix, baseCol, m_fontChat);
                DrawStringToHandle(x + 60, y + 8, log.message.c_str(), GetColor(255, 255, 255), m_fontChat);

                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
            else {
                // ネオン管のように発光する手書きスタンプ
                unsigned int lineCol = log.isMine ? GetColor(255, 165, 0) : GetColor(60, 150, 255);

                for (int i = 0; i < log.pointCount - 1; ++i) {
                    // 中心を白く、外側をぼかす（3重描画）
                    SetDrawBlendMode(DX_BLENDMODE_ADD, alpha / 3);
                    DrawLine(log.pointsX[i], log.pointsY[i], log.pointsX[i + 1], log.pointsY[i + 1], lineCol, 12);
                    SetDrawBlendMode(DX_BLENDMODE_ADD, alpha / 2);
                    DrawLine(log.pointsX[i], log.pointsY[i], log.pointsX[i + 1], log.pointsY[i + 1], lineCol, 6);
                    SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
                    DrawLine(log.pointsX[i], log.pointsY[i], log.pointsX[i + 1], log.pointsY[i + 1], GetColor(255, 255, 255), 2);
                }
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
        }

        // ==========================================
        // 2. 入力中UIの描画（ハッキングツール風）
        // ==========================================
        if (m_isTyping) {
            int bx = 40, by = 680, bw = 500, bh = 46;
            unsigned int uiCol = GetColor(0, 255, 255); // シアンでサイバー感を演出

            // 背景と枠線
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
            DrawBox(bx, by, bx + bw, by + bh, GetColor(5, 10, 15), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            DrawCyberBracket(bx, by, bw, bh, uiCol);

            // ブリンク（点滅）カーソル
            int blink = (GetNowCount() / 250) % 2;
            if (blink == 0) DrawBox(bx + 15, by + 12, bx + 22, by + 34, uiCol, TRUE);

            // ① 現在の入力モード（「あ」や「A」などのアイコン）を枠の少し上に表示する
            DrawKeyInputModeString(bx + 30, by - 20);

            // ② IMEの変換状態を含めたリアルタイム文字列描画（神関数）
            DrawKeyInputString(bx + 30, by + 13, m_inputStringEnd, TRUE); // ★ Mode を消す！
        }

        // 自分がドラッグして描いている最中のスタンプ
        if (m_isDrawing && m_currentDrawing.pointCount > 1) {
            for (int i = 0; i < m_currentDrawing.pointCount - 1; ++i) {
                SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
                DrawLine(m_currentDrawing.pointsX[i], m_currentDrawing.pointsY[i], m_currentDrawing.pointsX[i + 1], m_currentDrawing.pointsY[i + 1], GetColor(0, 255, 255), 6);
                SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
                DrawLine(m_currentDrawing.pointsX[i], m_currentDrawing.pointsY[i], m_currentDrawing.pointsX[i + 1], m_currentDrawing.pointsY[i + 1], GetColor(255, 255, 255), 2);
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

} // namespace App