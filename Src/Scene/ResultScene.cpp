#define NOMINMAX
#include "ResultScene.h"
#include <DxLib.h>
#include <cmath>
#include <string>
#include <algorithm>
#include "../../CyberGrid.h"
#include "../Manager/ProceduralAudio.h"

namespace App {

    ResultScene::ResultScene()
        : m_state(State::FADE_IN), m_frameCount(0), m_stateTimer(0)
        , m_psHandle(-1), m_cbHandle(-1)
        , m_fontTitle(-1), m_fontLabel(-1), m_fontNum(-1), m_fontRank(-1)
        , m_winner(1), m_p1Stats({ 0 }), m_p2Stats({ 0 })
        , m_p1FinalScore(0), m_p2FinalScore(0)
        , m_rankScale(5.0f), m_bgOffset(0.0f)
        , m_dispTime(0), m_dispTurns(0)
        , m_dispP1Moves(0), m_dispP2Moves(0)
        , m_dispP1Ops(0), m_dispP2Ops(0)
        , m_dispP1Dmg(0), m_dispP2Dmg(0)
        , m_dispP1Score(0), m_dispP2Score(0)
    {
    }

    ResultScene::~ResultScene() {
        Release();
    }

    void ResultScene::Init() {
        m_state = State::FADE_IN;
        m_frameCount = 0;
        m_stateTimer = 0;
        m_rankScale = 5.0f;
        m_bgOffset = 0.0f;

        m_dispTime = m_dispTurns = 0.0f;
        m_dispP1Moves = m_dispP2Moves = m_dispP1Ops = m_dispP2Ops = m_dispP1Dmg = m_dispP2Dmg = m_dispP1Score = m_dispP2Score = 0.0f;

        auto* sm = SceneManager::GetInstance();
        m_winner = sm->GetWinnerPlayer();
        m_p1Stats = sm->GetP1Stats();
        m_p2Stats = sm->GetP2Stats();

        CalculateScoreAndRank();

        m_fontTitle = CreateFontToHandle("BIZ UD明朝 Medium", 100, 3, DX_FONTTYPE_ANTIALIASING);
        m_fontLabel = CreateFontToHandle("BIZ UDゴシック", 28, 2, DX_FONTTYPE_ANTIALIASING);
        m_fontNum = CreateFontToHandle("BIZ UDゴシック", 42, 3, DX_FONTTYPE_ANTIALIASING);
        m_fontRank = CreateFontToHandle("HGP創英角ﾎﾟｯﾌﾟ体", 200, 5, DX_FONTTYPE_ANTIALIASING);

        if (m_psHandle == -1) {
            m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));
            m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        }
    }

    void ResultScene::CalculateScoreAndRank() {
        // 1. まずは純粋なプレイ内容（基礎点）を計算する
        auto calcBasePerformance = [&](const BattleStats& stats) {
            int timeBonus = std::max(0, 10000 - (stats.playTimeFrames / 1000) * 50);
            int turnPenalty = stats.totalTurns * 200;
            int damageBonus = stats.maxDamage * 500;
            int opsBonus = stats.totalOpsUsed * 100; // 演算子を多く使って工夫したボーナス

            return std::max(0, timeBonus - turnPenalty + damageBonus + opsBonus);
            };

        int p1Base = calcBasePerformance(m_p1Stats);
        int p2Base = calcBasePerformance(m_p2Stats);

        // 2. 勝敗に応じた特大ボーナスとペナルティ（★絶対に勝者が上回るようにする！）
        if (m_winner == 1) {
            m_p1FinalScore = p1Base + 50000; // 勝者は基礎点 ＋ 5万点の特大ボーナス
            m_p2FinalScore = p2Base / 3;     // 敗者はスコアを 1/3 に没収（絶対に勝者に届かない）
        }
        else if (m_winner == 2) {
            m_p2FinalScore = p2Base + 50000;
            m_p1FinalScore = p1Base / 3;
        }
        else {
            // 引き分けの場合
            m_p1FinalScore = p1Base;
            m_p2FinalScore = p2Base;
        }

        // 3. ランクの判定（★敗者は問答無用で「敗」になる）
        auto calcRank = [&](int score, bool isWinner, std::string& rankStr, unsigned int& rankCol) {
            if (!isWinner) {
                rankStr = "敗";
                rankCol = GetColor(100, 100, 120); // 敗北用の暗い色
                return;
            }

            // 勝者のみ、スコアに応じてS〜Dランクの栄誉を付与！
            if (score >= 80000) { rankStr = "S"; rankCol = GetColor(255, 215, 0); } // 金
            else if (score >= 65000) { rankStr = "A"; rankCol = GetColor(255, 100, 100); } // 赤
            else if (score >= 55000) { rankStr = "B"; rankCol = GetColor(100, 150, 255); } // 青
            else if (score >= 45000) { rankStr = "C"; rankCol = GetColor(100, 255, 100); } // 緑
            else { rankStr = "D"; rankCol = GetColor(200, 200, 200); } // 灰
            };

        calcRank(m_p1FinalScore, (m_winner == 1), m_p1Rank, m_p1RankCol);
        calcRank(m_p2FinalScore, (m_winner == 2), m_p2Rank, m_p2RankCol);
    }
    void ResultScene::Load() {}
    void ResultScene::LoadEnd() {}

    void ResultScene::Update() {
        m_frameCount++;
        m_stateTimer++;
        m_bgOffset += 2.0f;

        switch (m_state) {
        case State::FADE_IN:
            if (m_stateTimer > 30) {
                m_state = State::COUNT_STATS;
                m_stateTimer = 0;
            }
            break;

        case State::COUNT_STATS:
        {
            float p = m_stateTimer / 60.0f;
            if (p > 1.0f) p = 1.0f;
            float ease = 1.0f - std::pow(1.0f - p, 3.0f);

            m_dispTime = m_p1Stats.playTimeFrames * ease; // 時間とターンは共通
            m_dispTurns = m_p1Stats.totalTurns * ease;

            m_dispP1Moves = m_p1Stats.totalMoves * ease; m_dispP2Moves = m_p2Stats.totalMoves * ease;
            m_dispP1Ops = m_p1Stats.totalOpsUsed * ease; m_dispP2Ops = m_p2Stats.totalOpsUsed * ease;
            m_dispP1Dmg = m_p1Stats.maxDamage * ease;    m_dispP2Dmg = m_p2Stats.maxDamage * ease;

            if (m_frameCount % 5 == 0) ProceduralAudio::GetInstance().PlayPowerSE(2);

            if (m_stateTimer >= 60) {
                m_dispTime = (float)m_p1Stats.playTimeFrames; m_dispTurns = (float)m_p1Stats.totalTurns;
                m_dispP1Moves = (float)m_p1Stats.totalMoves; m_dispP2Moves = (float)m_p2Stats.totalMoves;
                m_dispP1Ops = (float)m_p1Stats.totalOpsUsed; m_dispP2Ops = (float)m_p2Stats.totalOpsUsed;
                m_dispP1Dmg = (float)m_p1Stats.maxDamage;    m_dispP2Dmg = (float)m_p2Stats.maxDamage;

                m_state = State::WAIT_SCORE;
                m_stateTimer = 0;
            }
            break;
        }
        case State::WAIT_SCORE:
            if (m_stateTimer >= 20) { m_state = State::COUNT_SCORE; m_stateTimer = 0; }
            break;
        case State::COUNT_SCORE:
        {
            float p = m_stateTimer / 90.0f;
            if (p > 1.0f) p = 1.0f;
            float ease = 1.0f - std::pow(1.0f - p, 3.0f);

            m_dispP1Score = m_p1FinalScore * ease;
            m_dispP2Score = m_p2FinalScore * ease;

            if (m_frameCount % 3 == 0) ProceduralAudio::GetInstance().PlayPowerSE(2);

            if (m_stateTimer >= 90) {
                m_dispP1Score = (float)m_p1FinalScore; m_dispP2Score = (float)m_p2FinalScore;
                m_state = State::RANK_STAMP; m_stateTimer = 0;
                ProceduralAudio::GetInstance().PlayPowerSE(6);
            }
            break;
        }
        case State::RANK_STAMP:
            m_rankScale += (1.0f - m_rankScale) * 0.2f;
            if (std::abs(1.0f - m_rankScale) < 0.05f) {
                m_rankScale = 1.0f;
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                m_state = State::WAIT_INPUT; m_stateTimer = 0;
            }
            break;
        case State::WAIT_INPUT:
            if (CheckHitKey(KEY_INPUT_SPACE) || CheckHitKey(KEY_INPUT_RETURN) || (GetMouseInput() & MOUSE_INPUT_LEFT)) {
                SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::TITLE);
                ProceduralAudio::GetInstance().PlayPowerSE(9);
            }
            break;
        }
    }

    void ResultScene::Draw() {
        int sw = 1920, sh = 1080;

        unsigned int themeCol1P = GetColor(255, 140, 0); // 1Pカラー（オレンジ）
        unsigned int themeCol2P = GetColor(0, 150, 255); // 2Pカラー（青）
        unsigned int winColor = (m_winner == 1) ? themeCol1P : (m_winner == 2) ? themeCol2P : GetColor(150, 150, 150);

        // ==========================================
        // 1. 背景描画
        // ==========================================
        DrawBox(0, 0, sw, sh, GetColor(5, 8, 15), TRUE);

        if (m_psHandle != -1 && m_cbHandle != -1) {
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            if (cb) {
                cb[0] = m_frameCount * 0.01f; cb[1] = (float)sw; cb[2] = (float)sh; cb[3] = 0.0f;
                UpdateShaderConstantBuffer(m_cbHandle);
                SetShaderConstantBuffer(m_cbHandle, DX_SHADERTYPE_PIXEL, 0);

                SetUsePixelShader(m_psHandle);
                VERTEX2DSHADER v[6];
                for (int i = 0; i < 6; ++i) {
                    v[i].pos = VGet(0, 0, 0); v[i].rhw = 1.0f;
                    v[i].dif = GetColorU8((winColor >> 16) & 0xFF, (winColor >> 8) & 0xFF, winColor & 0xFF, 255);
                    v[i].spc = GetColorU8(0, 0, 0, 0);
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
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210); DrawBox(0, 0, sw, sh, GetColor(0, 0, 0), TRUE); SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // ==========================================
        // 2. 勝者タイトルの描画
        // ==========================================
        std::string resultMain = (m_winner == 1) ? "1P VICTORY!!" : (m_winner == 2) ? "2P VICTORY!!" : "DRAW...";
        int textW = GetDrawStringWidthToHandle(resultMain.c_str(), (int)resultMain.length(), m_fontTitle);
        int titleX = sw / 2 - textW / 2;
        int titleY = 60;

        if (m_state != State::FADE_IN) {
            float t = m_frameCount * 0.1f;
            float glitchX = (std::sin(t * 15.0f) * std::cos(t * 22.0f)) * 8.0f;

            SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
            DrawStringToHandle(titleX + (int)glitchX, titleY, resultMain.c_str(), GetColor(255, 0, 100), m_fontTitle);
            DrawStringToHandle(titleX - (int)glitchX, titleY, resultMain.c_str(), GetColor(0, 150, 255), m_fontTitle);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawStringToHandle(titleX, titleY, resultMain.c_str(), winColor, m_fontTitle);
        }

        // ==========================================
        // 3. 戦績パネル (2人並びレイアウト！)
        // ==========================================
        int pX = sw / 2 - 600;
        int pY = 220;
        int pW = 1200;
        int pH = 450;

        if (m_state == State::FADE_IN) {
            float inEase = m_stateTimer / 30.0f;
            pY += (int)((1.0f - std::pow(1.0f - inEase, 3.0f)) * 100) - 100;
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(inEase * 255));
        }

        // 枠と背景
        DrawBox(pX, pY, pX + pW, pY + pH, GetColor(15, 18, 25), TRUE);
        DrawBox(pX, pY, pX + pW, pY + pH, GetColor(80, 80, 100), FALSE);

        // ヘッダー（1Pと2P）
        DrawStringToHandle(pX + pW / 4 - 30, pY + 20, "1P", themeCol1P, m_fontNum);
        DrawStringToHandle(pX + (pW * 3) / 4 - 30, pY + 20, "2P", themeCol2P, m_fontNum);
        DrawLine(pX + 20, pY + 70, pX + pW - 20, pY + 70, GetColor(80, 80, 100), 2);

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // ==========================================
        // 4. 各種項目の描画（中央に項目名、左右に値）
        // ==========================================
// ==========================================
        // 4. 各種項目の描画（中央に項目名、左右に値）
        // ==========================================
        auto drawCenterStat = [&](int yOffset, const char* label, int val1, int val2, const char* format = "%d") {
            int drawY = pY + yOffset;

            // 中央の項目名
            int lW = GetDrawStringWidthToHandle(label, (int)strlen(label), m_fontLabel);
            DrawStringToHandle(pX + pW / 2 - lW / 2, drawY, label, GetColor(150, 180, 200), m_fontLabel);

            char v1Str[64], v2Str[64];
            if (std::string(label) == "クリアタイム") {
                // 試合時間は両者共通！
                int sec = val1 / 1000;
                sprintf_s(v1Str, "%02d:%02d", sec / 60, sec % 60);
                strcpy_s(v2Str, v1Str);
            }
            else if (std::string(label) == "総ターン数") {
                // ターン数も両者共通！
                sprintf_s(v1Str, format, val1);
                strcpy_s(v2Str, v1Str);
            }
            else {
                // それ以外の個別ステータス
                sprintf_s(v1Str, format, val1);
                sprintf_s(v2Str, format, val2);
            }

            int v1W = GetDrawStringWidthToHandle(v1Str, (int)strlen(v1Str), m_fontNum);
            int v2W = GetDrawStringWidthToHandle(v2Str, (int)strlen(v2Str), m_fontNum);

            // ★修正：ハイフンによる省略をやめて、1Pにも2Pにもしっかり数値を表示！
            DrawStringToHandle(pX + pW / 4 - v1W / 2, drawY - 10, v1Str, GetColor(255, 255, 255), m_fontNum);
            DrawStringToHandle(pX + (pW * 3) / 4 - v2W / 2, drawY - 10, v2Str, GetColor(255, 255, 255), m_fontNum);

            // 区切り線
            DrawLine(pX + 50, drawY + 45, pX + pW - 50, drawY + 45, GetColor(40, 50, 60), 1);
            };
        if (m_state != State::FADE_IN) {
            drawCenterStat(90, "クリアタイム", (int)m_dispTime, 0);
            drawCenterStat(160, "総ターン数", (int)m_dispTurns, 0);
            drawCenterStat(230, "総移動マス数", (int)m_dispP1Moves, (int)m_dispP2Moves);
            drawCenterStat(300, "使用した演算子", (int)m_dispP1Ops, (int)m_dispP2Ops);
            drawCenterStat(370, "最大ダメージ", (int)m_dispP1Dmg, (int)m_dispP2Dmg);
        }

        // ==========================================
        // 5. 最終スコアパネルとランクスタンプ
        // ==========================================
        int sY = pY + pH + 30;
        DrawBox(pX, sY, pX + pW, sY + 120, GetColor(15, 18, 25), TRUE);
        DrawBox(pX, sY, pX + pW, sY + 120, winColor, FALSE);

        if (m_state != State::FADE_IN) {
            int lW = GetDrawStringWidthToHandle("最終スコア", 10, m_fontLabel);
            DrawStringToHandle(pX + pW / 2 - lW / 2, sY + 45, "最終スコア", GetColor(255, 255, 255), m_fontLabel);

            char s1[64], s2[64];
            sprintf_s(s1, "%08d", (int)m_dispP1Score);
            sprintf_s(s2, "%08d", (int)m_dispP2Score);

            int s1W = GetDrawStringWidthToHandle(s1, (int)strlen(s1), m_fontTitle);
            int s2W = GetDrawStringWidthToHandle(s2, (int)strlen(s2), m_fontTitle);

            DrawStringToHandle(pX + pW / 4 - s1W / 2 + 30, sY + 10, s1, themeCol1P, m_fontTitle);
            DrawStringToHandle(pX + (pW * 3) / 4 - s2W / 2 - 30, sY + 10, s2, themeCol2P, m_fontTitle);
        }

        if (m_state == State::RANK_STAMP || m_state == State::WAIT_INPUT) {
            // 1Pのスタンプ
            int r1W = GetDrawStringWidthToHandle(m_p1Rank.c_str(), (int)m_p1Rank.length(), m_fontRank);
            int d1X = (pX + 80) - (int)(r1W * m_rankScale) / 2;
            int d1Y = (sY + 60) - (int)(200 * m_rankScale) / 2;
            DrawExtendStringFToHandle((float)d1X, (float)d1Y, m_rankScale, m_rankScale, m_p1Rank.c_str(), m_p1RankCol, m_fontRank);

            // 2Pのスタンプ
            int r2W = GetDrawStringWidthToHandle(m_p2Rank.c_str(), (int)m_p2Rank.length(), m_fontRank);
            int d2X = (pX + pW - 80) - (int)(r2W * m_rankScale) / 2;
            int d2Y = (sY + 60) - (int)(200 * m_rankScale) / 2;
            DrawExtendStringFToHandle((float)d2X, (float)d2Y, m_rankScale, m_rankScale, m_p2Rank.c_str(), m_p2RankCol, m_fontRank);
        }

        // ==========================================
        // 6. 次へ進むプロンプト
        // ==========================================
        if (m_state == State::WAIT_INPUT) {
            int pulseAlpha = 150 + (int)(std::sin(m_frameCount / 10.0f) * 105);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, pulseAlpha);
            const char* prompt = ">> スペース か クリック で タイトル へ <<";
            int pW2 = GetDrawStringWidthToHandle(prompt, (int)strlen(prompt), m_fontLabel);
            DrawStringToHandle(sw / 2 - pW2 / 2, sh - 60, prompt, GetColor(255, 255, 255), m_fontLabel);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    void ResultScene::Release() {
        if (m_psHandle != -1) { DeleteShader(m_psHandle); m_psHandle = -1; }
        if (m_cbHandle != -1) { DeleteShaderConstantBuffer(m_cbHandle); m_cbHandle = -1; }
        if (m_fontTitle != -1) { DeleteFontToHandle(m_fontTitle); m_fontTitle = -1; }
        if (m_fontLabel != -1) { DeleteFontToHandle(m_fontLabel); m_fontLabel = -1; }
        if (m_fontNum != -1) { DeleteFontToHandle(m_fontNum); m_fontNum = -1; }
        if (m_fontRank != -1) { DeleteFontToHandle(m_fontRank); m_fontRank = -1; }
    }

} // namespace App